use std::sync::atomic::AtomicU8;
use std::time::Instant;

const PACKET_START: u8 = 0x4e;

const SYN: u8 = 0x01;
const SYN_ACK: u8 = 0x02;
const ACK: u8 = 0x03;
const PING: u8 = 0x04;
const PONG: u8 = 0x05;
const UPDATE: u8 = 0x06;

static PID: AtomicU8 = AtomicU8::new(1);

#[derive(Debug)]
struct Packet {
    pid: u8,
    typ: u8,
    data: Vec<u8>,
    checksum: u16,
}

impl Packet {}

fn parse_packet(port: &mut Box<dyn serialport::SerialPort>) -> Packet {
    let mut pid = None;
    let mut typ = None;
    let mut data_len = None;
    let mut data = None;
    let mut checksum = None;

    loop {
        // Read the header of the packet
        if port.bytes_to_read().unwrap() >= 3 {
            let mut buf = [0; 3];
            if let Ok(_) = port.read_exact(&mut buf) {
                pid = Some(buf[0]);
                typ = Some(buf[1]);
                data_len = Some(buf[2] as u32);
            }
        }

        if typ.is_some() {
            assert!(pid.is_some());
            assert!(typ.is_some());
            assert!(data_len.is_some());

            loop {
                if port.bytes_to_read().unwrap() >= data_len.unwrap() {
                    let mut buf = vec![0; data_len.unwrap() as usize];
                    if let Ok(_) = port.read_exact(&mut buf) {
                        data = Some(buf);
                        break;
                    }
                }
            }

            assert!(data.is_some());

            loop {
                if port.bytes_to_read().unwrap() >= 2 {
                    let mut buf = [0; 2];
                    if let Ok(_) = port.read_exact(&mut buf) {
                        let a = buf[0] as u16;
                        let b = buf[1] as u16;
                        checksum = Some(b << 8 | a);
                        break;
                    }
                }
            }

            assert!(checksum.is_some());

            break;
        }
    }

    return Packet {
        pid: pid.unwrap(),
        typ: typ.unwrap(),
        data: data.unwrap(),
        checksum: checksum.unwrap(),
    };
}

fn send_packet(
    port: &mut Box<dyn serialport::SerialPort>,
    typ: u8,
    data: &[u8],
) {
    let mut buf = Vec::new();

    let pid = PID.fetch_add(1, std::sync::atomic::Ordering::SeqCst);

    buf.push(PACKET_START);
    buf.push(pid);
    buf.push(typ);

    assert!(data.len() < 28);
    buf.push(data.len() as u8);
    buf.extend_from_slice(data);

    let checksum = tusk_rs::checksum(&buf[1..]);
    buf.extend_from_slice(&checksum.to_le_bytes());

    match port.write(&buf) {
        Ok(n) => {
            if n != buf.len() {
                eprintln!("Failed to write buffer?");
            }
        }
        Err(e) => eprintln!("Failed to send packet: {:?}", e),
    }
}

fn send_empty_packet(port: &mut Box<dyn serialport::SerialPort>, typ: u8) {
    send_packet(port, typ, &[]);
}

fn main() {
    let mut port = serialport::new("/dev/cu.usbserial-120", 9600)
        // .timeout(Duration::from_millis(5000))
        .open()
        .unwrap();

    let mut last = Instant::now();

    // Send SYN
    // Wait for SYN-ACK
    // If timeout is first then resend SYN
    // If SYN-ACK arrives first then send ACK

    let connection;
    send_empty_packet(&mut port, SYN);
    loop {
        if last.elapsed().as_millis() > 2000 {
            println!("Resend SYN");
            send_empty_packet(&mut port, SYN);
            last = Instant::now();
        }

        let mut buf = [0; 1];
        if let Ok(_) = port.read_exact(&mut buf) {
            if buf[0] == PACKET_START {
                let packet = parse_packet(&mut port);
                if packet.typ == SYN_ACK {
                    println!("Got SYN-ACK send ACK");
                    send_empty_packet(&mut port, ACK);
                    connection = true;
                    break;
                }
            }
        }
    }

    if connection {
        println!("Got connection");

        loop {
            let mut buf = [0; 1];
            if let Ok(_) = port.read_exact(&mut buf) {
                if buf[0] == PACKET_START {
                    let packet = parse_packet(&mut port);
                    if packet.typ == PING {
                        println!("Got ping");
                        send_empty_packet(&mut port, PONG);
                    }

                    if packet.typ == UPDATE {
                        println!("UPDATE: {:?}", packet);
                    }
                }
            }
        }
    }
}
