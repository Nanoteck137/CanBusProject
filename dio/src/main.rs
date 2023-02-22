use std::sync::atomic::AtomicU8;

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
    typ: u8,
}

impl Packet {}

fn parse_packet(port: &mut Box<dyn serialport::SerialPort>) -> Packet {
    let mut typ = None;

    loop {
        // Read the header of the packet
        if port.bytes_to_read().unwrap() >= 3 {
            let mut buf = [0; 3];
            if let Ok(_) = port.read_exact(&mut buf) {
                println!("Packet Header: {:?}", buf);

                let pid = buf[0];
                println!("PID: {}", pid);

                typ = Some(buf[1]);

                let data_len = buf[2];
                assert_eq!(data_len, 0);
            }
        }

        if typ.is_some() {
            loop {
                if port.bytes_to_read().unwrap() >= 2 {
                    println!("Waiting for checksum");
                    let mut buf = [0; 2];
                    if let Ok(_) = port.read_exact(&mut buf) {
                        println!("Packet Checksum: {:?}", buf);
                        break;
                    }
                }
            }

            break;
        }
    }

    return Packet { typ: typ.unwrap() };
}

fn send_syn_ack(port: &mut Box<dyn serialport::SerialPort>) {
    let mut buf: Vec<u8> = Vec::new();

    let pid = PID.fetch_add(1, std::sync::atomic::Ordering::SeqCst);

    buf.push(PACKET_START);
    buf.push(pid);
    buf.push(SYN_ACK);

    let checksum = tusk_rs::checksum(&buf);
    println!("Checksum: {:#x}", checksum);

    buf.extend_from_slice(&checksum.to_le_bytes());

    println!("Buf: {:x?}", buf);
    match port.write(&buf) {
        Ok(n) => {
            if n != buf.len() {
                eprintln!("Failed to print buffer?");
            }
        }
        Err(e) => eprintln!("Failed to send syn-ack: {:?}", e),
    }
}

fn send_ping(port: &mut Box<dyn serialport::SerialPort>) {
    let mut buf: Vec<u8> = Vec::new();

    let pid = PID.fetch_add(1, std::sync::atomic::Ordering::SeqCst);

    buf.push(PACKET_START);
    buf.push(pid);
    buf.push(PING);

    let checksum = tusk_rs::checksum(&buf);
    println!("Checksum: {:#x}", checksum);

    buf.extend_from_slice(&checksum.to_le_bytes());

    println!("Buf: {:x?}", buf);
    match port.write(&buf) {
        Ok(n) => {
            if n != buf.len() {
                eprintln!("Failed to print buffer?");
            }
        }
        Err(e) => eprintln!("Failed to send syn-ack: {:?}", e),
    }
}

fn main() {
    let mut port = serialport::new("/dev/cu.usbserial-120", 9600)
        // .timeout(Duration::from_millis(5000))
        .open()
        .unwrap();

    let mut got_syn = false;
    let mut connection = false;
    loop {
        let mut buf = [0; 1];
        if let Ok(_) = port.read_exact(&mut buf) {
            if buf[0] == PACKET_START {
                println!("Got Packet");
                let packet = parse_packet(&mut port);
                println!("Packet: {:?}", packet);

                match packet.typ {
                    SYN => {
                        got_syn = true;
                        println!("We need to send syn-ack");
                        send_syn_ack(&mut port);
                        println!("Sent syn-ack");
                    }

                    ACK => {
                        if got_syn {
                            connection = true;
                        }
                    }

                    _ => println!("Expceted SYN Packet"),
                }
            }
        }

        if connection {
            println!("Got connection");
            break;
        }
    }

    send_ping(&mut port);

    loop {
        let mut buf = [0; 1];
        if let Ok(_) = port.read_exact(&mut buf) {
            if buf[0] == PACKET_START {
                let packet = parse_packet(&mut port);
                if packet.typ == PONG {
                    println!("Got pong");
                }

                if packet.typ == UPDATE {
                    println!("UPDATE");
                }
            }
        }
    }
}
