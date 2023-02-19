use std::time::Duration;

fn main() {
    let ports = serialport::available_ports().unwrap();
    for port in &ports {
        println!("Port: {}", port.port_name);
    }

    let port = ports
        .iter()
        .find(|item| item.port_name == "COM4")
        .expect("Failed to find COM4");

    let mut port = serialport::new(port.port_name.clone(), 9600)
        .stop_bits(serialport::StopBits::One)
        .data_bits(serialport::DataBits::Eight)
        .parity(serialport::Parity::None)
        .timeout(Duration::from_millis(500))
        .open()
        .expect("Failed to open COM4");
    println!("Port Opened");
    let data = [0x00u8, 0x01u8];
    let length = port.write(&data).unwrap();
    println!("Length: {}", length);
    port.flush().unwrap();
    println!("Start Reading");
    // loop {
    //     println!("Test: {:?}", port.bytes_to_read());
    //     std::thread::sleep(Duration::from_millis(1000));
    // }
    let mut res = [0u8; 1024];
    let length = port.read(&mut res).unwrap();
    println!("Data: {:x?}", std::str::from_utf8(&res[0..length]));
}
