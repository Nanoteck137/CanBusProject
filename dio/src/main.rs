use std::io::Write;
use std::sync::atomic::AtomicU8;
use std::time::Duration;

use clap::{Parser, Subcommand};
use serialport::{SerialPortInfo, SerialPortType};

const PACKET_START: u8 = 0x4e;

const SYN: u8 = 0x01;
const SYN_ACK: u8 = 0x02;
const ACK: u8 = 0x03;
const PING: u8 = 0x04;
const PONG: u8 = 0x05;
const UPDATE: u8 = 0x06;

static PID: AtomicU8 = AtomicU8::new(1);

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[command(subcommand)]
    action: Action,
}

#[derive(Subcommand, Debug)]
enum Action {
    List,
    Debug {
        port: String,

        #[arg(short, long, default_value_t = 115200)]
        baudrate: u32,
    },

    // TODO(patrik): Better name?
    Run {
        port: String,

        #[arg(short, long, default_value_t = 115200)]
        baudrate: u32,
    },
}

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
    let data;
    let checksum;

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

fn print_port(port: &SerialPortInfo) {
    if let SerialPortType::UsbPort(info) = &port.port_type {
        // pub vid: u16,
        // pub pid: u16,
        // pub serial_number: Option<String>,
        // pub manufacturer: Option<String>,
        // pub product: Option<String>,

        if let Some(product) = &info.product {
            print!(" {}", product);
        }

        if let Some(manufacturer) = &info.manufacturer {
            print!(" ({})", manufacturer);
        }

        print!(" {}", port.port_name);
        println!();
    } else {
        panic!("Only USB based serial ports is supported");
    }
}

fn choose_port() -> SerialPortInfo {
    let ports = serialport::available_ports().unwrap();
    for port in &ports {
        println!("Port: {:#?}", port);
    }

    let ports = ports
        .iter()
        .filter(|i| {
            matches!(i.port_type, serialport::SerialPortType::UsbPort(..))
        })
        .collect::<Vec<_>>();

    for i in 0..ports.len() {
        let port = &ports[i];
        print!("{}:", i);
        print_port(port);
    }

    let index = loop {
        print!("Choose device: ");
        std::io::stdout().flush().unwrap();

        let mut buffer = String::new();
        std::io::stdin().read_line(&mut buffer).unwrap();

        match buffer.trim_end().parse::<usize>() {
            Ok(num) => break num,
            Err(_) => continue,
        }
    };

    ports[index].clone()
}

fn run_debug_monitor(port: &String, baudrate: u32) {
    let mut port = serialport::new(port, baudrate)
        .timeout(Duration::from_millis(5000))
        .open()
        .unwrap();

    loop {
        let mut buf = [0; 1];
        port.read(&mut buf).unwrap();
        std::io::stdout().write(&buf).unwrap();
    }
}

fn run(port: &String, baudrate: u32) {
    let mut port = serialport::new(port, baudrate)
        .timeout(Duration::from_millis(5000))
        .open()
        .unwrap();

    port.write(&[0xfe, 0xff]).unwrap();
}

fn main() {
    let args = Args::parse();

    match args.action {
        Action::List => {
            let ports = serialport::available_ports().unwrap();
            for port in ports {
                println!("{}", port.port_name);
            }

            return;
        }

        Action::Debug { port, baudrate } => run_debug_monitor(&port, baudrate),
        Action::Run { port, baudrate } => run(&port, baudrate),
    }

    // // TODO(patrik): Override the baud rate
    // let mut port = serialport::new(port.port_name.clone(), 115200)
    //     .timeout(Duration::from_millis(5000))
    //     .open()
    //     .unwrap();
    //
    // let mut last = Instant::now();
    //
    // let connection;
    // send_empty_packet(&mut port, SYN);
    // loop {
    //     if last.elapsed().as_millis() > 2000 {
    //         println!("Resend SYN");
    //         send_empty_packet(&mut port, SYN);
    //         last = Instant::now();
    //     }
    //
    //     let mut buf = [0; 1];
    //     if let Ok(_) = port.read_exact(&mut buf) {
    //         if buf[0] == PACKET_START {
    //             let packet = parse_packet(&mut port);
    //             if packet.typ == SYN_ACK {
    //                 println!("Got SYN-ACK send ACK");
    //                 send_empty_packet(&mut port, ACK);
    //                 connection = true;
    //                 break;
    //             }
    //         }
    //     }
    // }
    //
    // if connection {
    //     println!("Got connection");
    //
    //     loop {
    //         let mut buf = [0; 1];
    //         if let Ok(_) = port.read_exact(&mut buf) {
    //             if buf[0] == PACKET_START {
    //                 let packet = parse_packet(&mut port);
    //                 if packet.typ == PING {
    //                     println!("Got ping");
    //                     send_empty_packet(&mut port, PONG);
    //                 }
    //
    //                 if packet.typ == UPDATE {
    //                     println!("UPDATE: {:?}", packet);
    //                 }
    //             }
    //         }
    //     }
    // }
}
