use serialport;
use std::env;
fn main() {
    let args: Vec<String> = env::args().collect();
    let dev_path = if (args.size() > 0)
    {
        args[0]
    }
    else
    {
        panic!("Please provide path to port");
    };
    println!("Hello, world!");
    let port = serialport::new(dev_path, 115_200)
        .timeout(Duration::from_millis(10))
        .open().expect("Failed to open port");

    let mut serial_buffer: Vec<u8> = vec![0; 32];
    let mut running = true;
    while(running)
    {
        if Some(port.read(serial_buffer.as_mut_slice()))
        {
           // Handle data 
        }
    }
}
