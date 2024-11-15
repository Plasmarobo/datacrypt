use bitvec::bits;
use serialport;
use serialport::SerialPort;
use core::time;
use std::cell::RefCell;
use std::env;
use std::fmt;
use std::io::Write;
use std::io::{self, BufRead};
use std::rc::Rc;
use std::sync::{
    mpsc::{channel, Receiver, Sender, TryRecvError},
    Arc, RwLock,
};
use std::{
    fs::File,
    path::Path,
    thread::{self, JoinHandle},
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

#[macro_use]
extern crate scan_rules;

const MAX_BUFFER: usize = 64 + 6;

fn rpc_header(code: u8, address: u32, length: u8) -> Vec<u8> {
    let mut header = Vec::<u8>::new();
    header.push(code);
    header.extend(address.to_le_bytes());
    header.extend(length.to_le_bytes());
    header
}

trait Serial {
    fn read(&mut self) -> Vec<u8>;
    fn try_read(&mut self) -> Option<Vec<u8>>;
    fn write(&mut self, data: Vec<u8>);
    fn clear(&mut self);
}

#[derive(fmt::Debug)]
struct AsyncSerial {
    tx: Sender<Vec<u8>>,
    rx: Receiver<Vec<u8>>,
    notify: Arc<RwLock<bool>>,
    worker: Option<JoinHandle<()>>,
}

impl AsyncSerial {
    pub fn new(dev_path: String) -> AsyncSerial {
        let (tx_app, tx_port) = channel::<Vec<u8>>();
        let (rx_port, rx_app) = channel::<Vec<u8>>();
        let notify = Arc::new(RwLock::new(false));
        let mut t_port = serialport::new(dev_path.clone(), 115_200)
            .timeout(Duration::from_millis(10))
            .open()
            .expect("Failed to open port");
        t_port
            .clear(serialport::ClearBuffer::All)
            .expect("Failed to clear serial buffer");
        let rx_notify = Arc::clone(&notify);
        let tx_notify = Arc::clone(&notify);
        let worker_tx = thread::spawn(move || {
            let tx_thread = std::thread::current();
            let mut r_port = t_port.try_clone().expect("Could not clone port");
            let rx_handle = thread::spawn(move || loop {
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
                    Err(e) => eprintln!("{:?}", e),
                }
                tx_thread.unpark();
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
                        //println!("{:?}", buffer);
                        t_port
                            .write_all(buffer.as_slice())
                            .expect("failed to write to serialport")
                    }
                    Err(TryRecvError::Empty) => (),
                    Err(e) => {
                        panic!("failed to recv from app: {:?}", e)
                    }
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
            worker: Some(worker_tx),
        }
    }
}
impl Serial for AsyncSerial {
    fn read(&mut self) -> Vec<u8> {
        let mut data = Vec::<u8>::new();
        data.extend(self.rx.recv().expect("failed to recv from port"));
        return data;
    }

    fn try_read(&mut self) -> Option<Vec<u8>> {
        match self.rx.try_recv() {
            Ok(data) => Some(data),
            Err(TryRecvError::Empty) => None,
            Err(e) => {
                panic!("failed to recv from port, unrecoverable {:?}", e)
            }
        }
    }

    fn write(&mut self, data: Vec<u8>) {
        self.tx.send(data).expect("failed to send to port");
    }

    fn clear(&mut self)
    {
        self.try_read();
    }
}

impl Drop for AsyncSerial {
    fn drop(&mut self) {
        *(self.notify.write().unwrap()) = true;
        let _ = self.worker.take().unwrap().join();
    }
}

struct SyncSerial {
    port: Box<dyn SerialPort>,
}

impl SyncSerial {
    pub fn new(dev_path: String) -> SyncSerial {
        let s_port = serialport::new(dev_path.clone(), 115_200)
            .timeout(Duration::from_millis(10))
            .open()
            .expect("Failed to open port");
        s_port
            .clear(serialport::ClearBuffer::All)
            .expect("Failed to clear serial buffer");
        SyncSerial { port: s_port }
    }
}

impl Serial for SyncSerial {
    fn read(&mut self) -> Vec<u8> {
        let mut buffer: Vec<u8> = vec![0; MAX_BUFFER];
        match self.port.read(buffer.as_mut_slice()) {
            Ok(_available_count) => buffer.resize(_available_count, 0),
            Err(ref e) if e.kind() == io::ErrorKind::TimedOut => buffer.resize(0, 0),
            Err(e) => eprintln!("{:?}", e),
        }
        return buffer;
    }

    fn try_read(&mut self) -> Option<Vec<u8>> {
        let buffer = self.read();
        if buffer.len() > 0 {
            return Some(buffer);
        }
        None
    }

    fn write(&mut self, data: Vec<u8>) {
        self.port
            .write_all(data.as_slice())
            .expect("failed to write to serialport");
    }

    fn clear(&mut self)
    {
        self.port.clear(serialport::ClearBuffer::All);
    }
}

#[derive(fmt::Debug)]
enum RPCStatus {
    OK,
    BUSY,
    ERR_EXEC,
    ERR_ARG,
    ERR_TIMEOUT,
    ERR_HOST_TIMEOUT,
    ERR_HOST_CONNECTION,
    ERR_UNKNOWN,
}

impl fmt::Display for RPCStatus {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "{}",
            match self {
                RPCStatus::OK => "0 (RPC_OK)",
                RPCStatus::BUSY => "1 (BUSY)",
                RPCStatus::ERR_EXEC => "2 (EXEC ERROR)",
                RPCStatus::ERR_ARG => "3 (ARGUMENT ERROR)",
                RPCStatus::ERR_TIMEOUT => "4 (RPC TIMEOUT)",
                RPCStatus::ERR_HOST_TIMEOUT => "5 (HOST TIMEOUT)",
                _ => "_ (UNKNOWN ERROR)",
            }
        )
    }
}

struct RPCMessenger {
    serial: Box<dyn Serial>,
    buffer: Vec<u8>,
}

impl RPCMessenger {
    const RPC_CHUNK: u8 = 64;
    const MAX_DISPLAY: u8 = 7;
    pub fn new(dev: Box<dyn Serial>) -> RPCMessenger {
        RPCMessenger {
            serial: dev,
            buffer: Vec::<u8>::new(),
        }
    }

    fn get_status(&mut self) -> Result<(), RPCStatus> {
        const TIMEOUT_MS: u128 = 30000;
        let start = Instant::now();
        while self.buffer.len() < 1
        {
            if let Some(serial_data) = self.serial.try_read() {
                self.buffer.extend(&serial_data);
            }
            if (Instant::now() - start).as_millis() > TIMEOUT_MS {
                return Err(RPCStatus::ERR_HOST_TIMEOUT);
            }
        }
        let status = self.buffer[0];
        self.buffer = self.buffer[1..].to_vec();
        if status == 0
        {
            return Ok(());
        } else {
            return Err(match status {
                1 => RPCStatus::BUSY,
                2 => RPCStatus::ERR_EXEC,
                3 => RPCStatus::ERR_ARG,
                4 => RPCStatus::ERR_TIMEOUT,
                _ => RPCStatus::ERR_UNKNOWN,
            });
        }
    }

    fn get_data(&mut self, length: u8) -> Result<Vec<u8>, String> {
        const TIMEOUT_MS: u128 = 300;
        let start = Instant::now();
        while self.buffer.len() < length as usize {
            if let Some(serial_data) = self.serial.try_read() {
                self.buffer.extend(serial_data);
            }
            if (Instant::now() - start).as_millis() > TIMEOUT_MS {
                return Err(format!(
                    "Received {} bytes of an expected {}",
                    self.buffer.len(),
                    length
                ));
            }
        }
        let result = self.buffer[0..length as usize].to_vec();
        self.buffer = self.buffer[length as usize..].to_vec();
        Ok(result)
    }

    pub fn read_flash(&mut self, address: u32, length: u8) -> Result<Vec<u8>, RPCStatus> {
        self.buffer.clear();
        self.serial.clear();
        if length > RPCMessenger::RPC_CHUNK {
            Err(RPCStatus::ERR_ARG)
        } else {
            self.serial.write(rpc_header('r' as u8, address, length));
            match self.get_status() {
                Ok(()) => (),
                Err(e) => return Err(e),
            }
            let mut flash_buffer = Vec::<u8>::new();
            match self.get_data(length) {
                Ok(data) => flash_buffer.extend(data),
                Err(e) => {
                    println!("WARNING, getting data failed: {}", e);
                    return Err(RPCStatus::ERR_HOST_CONNECTION);
                },
            }
            match self.get_status() {
                Ok(()) => (),
                Err(e) => 
                {
                    println!("WARNING, getting finish status code failed: {}", e);
                    return Err(RPCStatus::ERR_HOST_CONNECTION);
                }
            }
            Ok(flash_buffer)
        }
    }

    pub fn write_flash(&mut self, address: u32, data: &[u8]) -> Result<(), RPCStatus> {
        self.buffer.clear();
        self.serial.clear();
        if (data.len() as u8 > RPCMessenger::RPC_CHUNK) {
            Err(RPCStatus::ERR_ARG)
        } else {
            self.serial
                .write(rpc_header('w' as u8, address, data.len() as u8));
            match self.get_status() {
                Ok(()) => (),
                Err(e) => return Err(e),
            }
            self.serial.write(data.to_vec());
            self.get_status()
        }
    }

    pub fn commit_flash(&mut self) -> Result<(), RPCStatus> {
        self.buffer.clear();
        self.serial.clear();
        self.serial.write(rpc_header('c' as u8, 0, 0));
        self.get_status()
    }

    pub fn erase_flash(&mut self, address: u32) -> Result<(), RPCStatus> {
        self.buffer.clear();
        self.serial.clear();
        self.serial.write(rpc_header('e' as u8, address, 0));
        self.get_status()
    }

    pub fn echo(&mut self, address: u8, data: &[u8]) -> Result<(), RPCStatus> {
        self.buffer.clear();
        self.serial.clear();
        if address > RPCMessenger::MAX_DISPLAY {
            return Err(RPCStatus::ERR_ARG);
        }
        if data.len() as u8 > RPCMessenger::RPC_CHUNK {
            return Err(RPCStatus::ERR_ARG);
        }
        self.serial
            .write(rpc_header('p' as u8, address as u32, data.len() as u8));
        match self.get_status() {
            Ok(()) => (),
            Err(e) => return Err(e),
        }
        self.serial.write(data.to_vec());
        self.get_status()
    }

    pub fn info(&mut self, address: u8) -> Result<(), RPCStatus> {
        self.buffer.clear();
        self.serial.clear();
        if address > RPCMessenger::MAX_DISPLAY {
            return Err(RPCStatus::ERR_ARG);
        }
        self.serial.write(rpc_header('i' as u8, address as u32, 0));
        self.get_status()
    }
}

fn dump_flash(rpc: Rc<RefCell<RPCMessenger>>) {
    const PAGE_COUNT: usize = 64 * 1024;
    const PAGE_SIZE: usize = 2048;
    const OOB_SIZE: usize = 64;
    const FLASH_SIZE: usize = 1024 * (2048 + 64);
    const CHUNK_SIZE: u8 = 64;
    let mut page_idx: u32 = 0;
    let mut byte_idx: u32 = 0;
    let mut file = File::create(format!(
        "flash_dump_{:?}.bin",
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs()
    ))
    .unwrap();
    println!("Starting flash dump");
    while page_idx < (PAGE_COUNT as u32) {
        while byte_idx < ((PAGE_SIZE + OOB_SIZE) as u32) {
            // Start a time out
            let address: u32 = page_idx << 16 | byte_idx;
            loop {
                thread::sleep(Duration::from_micros(500));
                match rpc.borrow_mut().read_flash(address, CHUNK_SIZE) {
                    Ok(data) => {
                        let kind = if byte_idx >= (PAGE_SIZE as u32) {
                            "OOB"
                        } else {
                            "DAT"
                        }
                        .to_string();
                        println!("{} {:#10x}: {:?}", kind, address, data);
                        file.write(format!("{:#10x}: ", address).as_bytes());
                        file.write(format!("{:X?}",data).as_bytes());
                        file.write("\n".as_bytes());
                        byte_idx += CHUNK_SIZE as u32;
                        break;
                    }
                    Err(e) => {
                        println!("Read of {} returned status {}", address, e);
                        thread::sleep(Duration::from_millis(500));
                    }
                }
            }
        }
        page_idx += 1;
        byte_idx = 0;
    }
}

fn dump_factory_bbt(rpc: Rc<RefCell<RPCMessenger>>) {
    const BLOCK_COUNT: usize = 1024;
    const OOB_SIZE: usize = 64;
    const CHUNK_SIZE: u8 = 64;
    const TIMEOUT_SEC: u64 = 5;
    let mut block_idx: u32 = 0;
    let byte_idx: u32 = 2048;
    let mut file = File::create(format!(
        "factory_bbt_dump_{:?}.bin",
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs()
    ))
    .unwrap();
    let bad_block_table = bits![mut 0; 1024];
    println!("Starting flash dump");
    while block_idx < (BLOCK_COUNT as u32) {
        for page_idx in 0..2 {
            println!("Block {}, page {}", block_idx, page_idx);
            // Start a time out
            let address: u32 = ((block_idx << 6) + page_idx) << 16 | byte_idx;
            loop {
                thread::sleep(Duration::from_micros(500));
                match rpc.borrow_mut().read_flash(address, CHUNK_SIZE) {
                    Ok(data) => {
                        if data[0..2] != *("ff".as_bytes()) {
                            let bit = bad_block_table.get_mut(block_idx as usize).unwrap();
                            bit.commit(true);
                            println!("OOB {:#10x}: {:?}", address, data);
                        }
                        file.write(data.as_slice());
                        break;
                    }
                    Err(e) => {
                        println!("Read of {} returned status {}", address, e);
                        thread::sleep(Duration::from_millis(500));
                    }
                }
            }
        }
        block_idx += 1;
    }
    println!("BBT: {}", bad_block_table);
}

struct WordBuffer {
    rpc: Rc<RefCell<RPCMessenger>>,
    write_buffer: Vec<u8>,
    page_idx: u32,
    byte_idx: u32,
}

impl WordBuffer {
    const PAGE_SIZE: usize = 2048;
    const CHUNK_SIZE: usize = 64;
    // Logical wordlist start address
    const WORDLIST_BASE: u32 = 0x00000800;
    const WORDLIST_PITCH: usize = 16; // Words are a maximum of 16 bytes long
    const WORDLIST_SLOTS_PER_PAGE: usize = WordBuffer::PAGE_SIZE / WordBuffer::WORDLIST_PITCH;

    pub fn new(_rpc: Rc<RefCell<RPCMessenger>>) -> WordBuffer {
        return WordBuffer {
            rpc: _rpc,
            write_buffer: Vec::<u8>::new(),
            page_idx: 1,
            byte_idx: 4,
        };
    }

    pub fn add_word(&mut self, word: String) {
        self.write_buffer.extend(word.trim().as_bytes());
        let remaining_length: usize = WordBuffer::WORDLIST_PITCH - word.len() - if self.page_idx == 1 { 4 } else { 0 };
        if remaining_length > 0 {
            self.write_buffer.extend(vec![0; remaining_length]);
        }
        if self.write_buffer.len() >= WordBuffer::PAGE_SIZE {
            self.commit();
        }
    }

    pub fn add_header(&mut self, count: u32) {
        match self
            .rpc
            .borrow_mut()
            .write_flash(self.page_idx << 16, &count.to_le_bytes())
        {
            Ok(()) => println!("Wrote {} bytes", 4),
            Err(e) => panic!("Error writing header: {}", e),
        }
    }

    pub fn commit(&mut self) {
        // Commit in 64 byte chunks (limit of the rpc interface)
        // Fill page of 2048 bytes before
        // use a read -> modify -> write flow
        // Discard this read result, it's just used to populate the buffer
        for buffer in self.write_buffer.chunks(WordBuffer::CHUNK_SIZE) {
            let mut retry = 3;
            let address: u32 = (self.page_idx << 16) | self.byte_idx;
            println!(
                "Commiting {} bytes at page {}, offset {}",
                WordBuffer::CHUNK_SIZE,
                self.page_idx,
                self.byte_idx
            );
            while retry > 0 {
                match self.rpc.borrow_mut().write_flash(address, buffer) {
                    Ok(()) => {
                        println!("Wrote {} bytes", WordBuffer::CHUNK_SIZE);
                        retry = 0;
                        self.byte_idx += buffer.len() as u32;
                    }
                    Err(e) => {
                        println!("Write return status: {}", e);
                        retry -= 1;
                    }
                }
            }
            if self.byte_idx as usize >= WordBuffer::PAGE_SIZE {
                // Flush to flash
                match self.rpc.borrow_mut().commit_flash() {
                    Ok(()) => (),
                    Err(e) => panic!(
                        "Error commmiting page {} to flash, got status {}",
                        self.page_idx, e
                    ),
                }
                self.page_idx += 1;
                self.byte_idx = 0;
                match self
                    .rpc
                    .borrow_mut()
                    .read_flash(self.page_idx << 16, WordBuffer::CHUNK_SIZE as u8)
                {
                    Ok(_data) => (),
                    Err(e) => println!("WARNING: failed to read next page with status {}", e),
                }
            }
        }
        self.write_buffer.clear();
    }
}

fn load_wordlist(rpc: Rc<RefCell<RPCMessenger>>) {
    let mut flash_word_buffer = WordBuffer::new(rpc);
    let word_count: u32 =
        io::BufReader::new(File::open(Path::new("../../data/decrypto.txt")).unwrap())
            .lines()
            .count() as u32;
    let lines = io::BufReader::new(File::open(Path::new("../../data/decrypto.txt")).unwrap())
        .lines()
        .flatten();
    flash_word_buffer.add_header(word_count);
    for line in lines {
        flash_word_buffer.add_word(line);
    }
    flash_word_buffer.commit();
    println!("Write finished");
}

fn erase_flash(rpc: Rc<RefCell<RPCMessenger>>) {
    const BLOCK_COUNT: usize = 1024;
    const OOB_SIZE: usize = 64;
    const CHUNK_SIZE: u8 = 64;
    const TIMEOUT_SEC: u64 = 5;
    let mut block_idx: u32 = 0;
    let byte_idx: u32 = 0;
    let bad_block_table = bits![mut 0; 1024];
    println!("Starting flash erase");
    while block_idx < (BLOCK_COUNT as u32) {
        
        print!("Block {}...", block_idx);
        // Start a time out
        let address: u32 = ((block_idx << 6)) << 16 | byte_idx;
        loop {
            thread::sleep(Duration::from_micros(500));
            match rpc.borrow_mut().erase_flash(address) {
                Ok(_) => {
                    println!("Erased!");
                    block_idx += 1;
                    break;
                }
                Err(e) => {
                    println!("Failed at {}, returned status {}", address, e);
                    thread::sleep(Duration::from_millis(500));
                }
            }
        }
    }
    println!("Erase complete");
}

fn main() {
    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);
    let dev_path = if args.len() > 1 {
        args[1].clone()
    } else {
        "COM21".to_string()
        //panic!("Please provide path to port");
    };
    //let serial = AsyncSerial::new(dev_path);
    let rpc = Rc::new(RefCell::new(RPCMessenger::new(Box::new(SyncSerial::new(
        dev_path,
    )))));
    //dump_flash(rpc.clone());
    //dump_factory_bbt(rpc.clone());
    //erase_flash(rpc.clone());
    load_wordlist(rpc.clone());

    loop {
        println!("Enter command");
        readln! {
            ("readflash", " ", let address: u32, " ", let size: u8) => {
                match rpc.borrow_mut().read_flash(address, size)
                {
                    Ok(data) => {
                        println!("Read {} bytes from {}", size, address);
                        for byte in data
                        {
                            print!("{:#04?} ", byte);
                        }
                        println!("");
                    },
                    Err(e) => println!("Got status {}", e)
                }
            },
            ("writeflash", " ", let address: u32, " ", [ let data: u8, " "]+) => {
                // Should parse hex from string or something here...
                // data.map(thing).collect()
                match rpc.borrow_mut().write_flash(address, data.as_slice())
                {
                    Ok(()) => println!("Wrote {} bytes", data.len()),
                    Err(e) => println!("Got status {}", e)
                }
            },
            ("commitflash") => {
                match rpc.borrow_mut().commit_flash()
                {
                    Ok(()) => println!("Commited flash"),
                    Err(e) => println!("Got status {}", e),
                }
            },
            ("eraseflash", " ", let address: u32) => {
                match rpc.borrow_mut().erase_flash(address)
                {
                    Ok(()) => println!("Erased block at {}", address),
                    Err(e) => println!("Got status {}", e),
                }
            },
            ("echo", " ", let display: u8, " ", let message: String) => {
                match rpc.borrow_mut().echo(display, message.as_bytes())
                {
                    Ok(()) => println!("Echoing {} to display {}", message, display),
                    Err(e) => println!("Got status {}", e),
                }
            },
            ("info", " ", let display: u8) => {
                match rpc.borrow_mut().info(display)
                {
                    Ok(()) => println!("Info shown on display {}", display),
                    Err(e) => println!("Got status {}", e),
                }
            },
            (.._) => println!("Unknown command"),
        };
    }
}
