use serialport;
use std::env;
use std::time::Duration;
use std::fmt::Debug;
use std::io::Write;
use std::sync::{mpsc::{channel, Sender, Receiver, TryRecvError}, RwLock, Arc};
use std::{thread::{self, JoinHandle}};
use std::io;

#[macro_use] extern crate scan_rules;

const MAX_BUFFER:usize = 512;

fn rpc_header(code: u8, address: u32, length:u8, data:Option<Vec<u8>>) -> Vec<u8>
{
    let mut header = Vec::<u8>::new();
    header.push(code);
    header.extend(address.to_le_bytes());
    header.extend(length.to_le_bytes());
    if let Some(data) = data {
        header.extend(data);
    }
    header
}

#[derive(Debug)]
struct AsyncSerial
{
    tx:Sender<Vec<u8>>,
    rx:Receiver<Vec<u8>>,
    notify:Arc<RwLock<bool>>,
    worker:Option<JoinHandle<()>>
}

impl AsyncSerial
{
    pub fn new(dev_path:String) -> AsyncSerial
    {
        let (tx_app, tx_port) = channel::<Vec<u8>>();
        let (rx_port, rx_app) = channel::<Vec<u8>>();
        let notify = Arc::new(RwLock::new(false));
        let mut t_port = serialport::new(dev_path.clone(), 115_200)
            .timeout(Duration::from_millis(10))
            .open().expect("Failed ot open port");

        let rx_notify = Arc::clone(&notify);
        let tx_notify = Arc::clone(&notify);
        let worker_tx = thread::spawn(move ||{
            let tx_thread = std::thread::current();
            let mut r_port = t_port.try_clone().expect("Could not clone port");
            let rx_handle = thread::spawn(move || {
                loop
                {
                    std::thread::park();
                    {
                        let terminate = rx_notify.read().unwrap();
                        if *terminate {
                            break;
                        }
                    }
                    let mut buffer: Vec<u8> = vec![0; MAX_BUFFER];
                    match r_port.read(buffer.as_mut_slice()) {
                        Ok(_available_count) => rx_port.send(buffer).expect("failed to send to app"),
                        Err(ref e) if e.kind() == io::ErrorKind::TimedOut => (),
                        Err(e) => eprintln!("{:?}", e)
                    }
                    tx_thread.unpark();
                    
                }
            });
            
            loop {
                {
                    let terminate = tx_notify.read().unwrap();
                    if *terminate {
                        break;
                    }
                }

                match tx_port.try_recv() {
                    Ok(buffer) => {
                        t_port.write_all(buffer.as_slice()).expect("failed to write to serialport")
                    }
                    Err(TryRecvError::Empty) => (),
                    Err(e) => {panic!("failed to recv from app: {:?}", e)}
                }
                rx_handle.thread().unpark(); // unpark read, then park ourself
                std::thread::park();
            }

            let _ = rx_handle.join();
        });
        AsyncSerial {
            tx: tx_app,
            rx: rx_app,
            notify: notify,
            worker: Some(worker_tx)
        }
    }

    pub fn read(&self) -> Vec<u8>
    {
        let mut data = Vec::<u8>::new();
        data.extend(self.rx.recv().expect("failed to recv from port"));
        return data;
    }

    pub fn try_read(&self) -> Option<Vec<u8>>
    {
        match self.rx.try_recv() {
            Ok(data) => Some(data),
            Err(TryRecvError::Empty) => None,
            Err(e) => {panic!("failed to recv from port, unrecoverable {:?}", e)}
        }
    }

    pub fn write(&self, data: Vec<u8>)
    {
        self.tx.send(data).expect("failed to send to port");
    }
}

impl Drop for AsyncSerial
{
    fn drop(&mut self) {
        *(self.notify.write().unwrap()) = true;
        let _ = self.worker.take().unwrap().join();
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);
    let dev_path = if args.len() > 1
    {
        args[1].clone()
    }
    else
    {
        panic!("Please provide path to port");
    };
    let serial = AsyncSerial::new(dev_path);
    let (command_tx, command_rx) = channel();

    thread::spawn(move || {
        loop {
            println!("Enter command");
            readln! {
                ("readflash", " ", let address: u32, " ", let size: u8) => { command_tx.send(rpc_header('r' as u8, address, size, None)).expect("Could not queue send");},
                ("writeflash", " ", let address: u32, " ", let size: u8, [ let data: u8, " "]+) => { command_tx.send(rpc_header('w' as u8, address, size, Some(data))).expect("Could not queue send");},
                ("eraseflash", " ", let address: u32, " ", let size: u8) => { command_tx.send(rpc_header('e' as u8, address, size, None)).expect("Could not queue send");},
                ("echo", " ", let message: String) => {command_tx.send(rpc_header('p' as u8, 0, message.len().to_le_bytes()[0], Some(message.as_bytes().to_vec()))).expect("Could not queue send");},
                ("info") => {command_tx.send(rpc_header('i' as u8, 0, 0, None)).expect("Could not queue send");},
                (.._) => println!("Unknown command"),
            };
        }
    });
    loop
    {
        match command_rx.try_recv() {
            Ok(data) => {
                serial.write(data);
            }
            Err(TryRecvError::Empty) => (),
            Err(e) => panic!("{:?}", e)
        }
        if let Some(data) = serial.try_read() {
            //println!("{:?}", data);
            println!("{}", std::str::from_utf8(data.as_slice()).expect("Invaild UTF-8 sequence"));
        }
    }
}
