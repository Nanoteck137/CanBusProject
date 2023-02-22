use std::time::Duration;

fn main() {
    let mut port = serialport::new("/dev/cu.usbserial-120", 9600)
        .timeout(Duration::from_millis(1000))
        .open()
        .unwrap();

    loop {
        port.write(&[0]).unwrap();

        let mut buf = [0; 1024];
        match port.read(&mut buf) {
            Ok(n) => println!("Read: {:?}", std::str::from_utf8(&buf[..n])),
            Err(e) => eprintln!("Error: {:?}", e),
        }

        std::thread::sleep(Duration::from_millis(500));
    }

    // let mut port = serial2::SerialPort::open("/dev/cu.usbserial-120", 9600).unwrap();
    // port.set_read_timeout(Duration::from_millis(500)).unwrap();
    // port.set_rts(true).unwrap();
    //
    // std::thread::sleep(Duration::from_millis(1000));
    //
    // let length = port.write(b"a").unwrap();
    // println!("Length: {}", length);
    // port.flush().unwrap();
    //
    // let mut buf = [0; 1024];
    // let len = port.read(&mut buf);
    // println!("Test: {:?}", len);
    //
    // println!("Start");
    // loop {
    //     port.write(b"a").unwrap();
    //     port.flush().unwrap();
    //
    //     let mut buf = [0; 1024];
    //     match port.read(&mut buf) {
    //         Ok(n) => {
    //             println!("Buf: {:?}", &buf[..n]);
    //         }
    //         Err(e) => eprintln!("Error: {:?}", e),
    //     }
    //
    //     std::thread::sleep(Duration::from_millis(500));
    // }
}
