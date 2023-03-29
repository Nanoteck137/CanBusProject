use std::io::{BufRead, Write};
use std::sync::atomic::AtomicU8;
use std::time::Duration;

use clap::{Parser, Subcommand};
use serialport::{SerialPortInfo, SerialPortType};
use tusk_rs::{PacketType, PACKET_START};

static PID: AtomicU8 = AtomicU8::new(1);

#[derive(Debug)]
enum Command {
    Identify,
    SetDeviceControls { device: u8, controls: u8 },
    GetDeviceControls { device: u8 },
    GetDeviceLines { device: u8 },
}

fn parse_u8(s: &str) -> Option<u8> {
    if s.len() >= 2 {
        if &s[0..2] == "0b" {
            Some(u8::from_str_radix(&s[2..], 2).ok()?)
        } else if &s[0..2] == "0x" {
            Some(u8::from_str_radix(&s[2..], 16).ok()?)
        } else {
            Some(u8::from_str_radix(s, 10).ok()?)
        }
    } else {
        Some(u8::from_str_radix(s, 10).ok()?)
    }
}

fn parse_cmd(cmd: &str) -> Option<Command> {
    let mut split = cmd.split(' ');

    let cmd = split.next()?;
    println!("Cmd: {}", cmd);

    match cmd {
        "identify" => Some(Command::Identify),

        "set_device_controls" => {
            // set_device_controls 0 0b00000000
            let device = split.next()?;
            let device = parse_u8(device)?;

            let controls = split.next()?;
            let controls = parse_u8(controls)?;

            Some(Command::SetDeviceControls { device, controls })
        }

        "get_device_controls" => {
            // get_device_controls 0
            let device = split.next()?;
            let device = parse_u8(device)?;

            Some(Command::GetDeviceControls { device })
        }

        "get_device_lines" => {
            // get_device_lines 0
            let device = split.next()?;
            let device = parse_u8(device)?;

            Some(Command::GetDeviceLines { device })
        }

        _ => None,
    }
}

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
    typ: PacketType,
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
        typ: PacketType::try_from(typ.unwrap()).unwrap(),
        data: data.unwrap(),
        checksum: checksum.unwrap(),
    };
}

fn send_packet(
    port: &mut Box<dyn serialport::SerialPort>,
    typ: PacketType,
    data: &[u8],
) {
    let mut buf = Vec::new();

    let pid = PID.fetch_add(1, std::sync::atomic::Ordering::SeqCst);

    buf.push(PACKET_START);
    buf.push(pid);
    buf.push(typ.to_u8());

    // assert!(data.len() < 28);
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

fn send_empty_packet(
    port: &mut Box<dyn serialport::SerialPort>,
    typ: PacketType,
) {
    send_packet(port, typ, &[]);
}

fn print_port(port: &SerialPortInfo) {
    if let SerialPortType::UsbPort(info) = &port.port_type {
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

fn wait_for_packet(port: &mut Box<dyn serialport::SerialPort>) -> Packet {
    loop {
        let mut buf = [0; 1];
        if let Ok(_) = port.read_exact(&mut buf) {
            if buf[0] == PACKET_START {
                let packet = parse_packet(port);
                return packet;
            }
        }
    }
}

struct Version(u16);

impl Version {
    fn major(&self) -> u8 {
        ((self.0 >> 10) & 0x3f) as u8
    }

    fn minor(&self) -> u8 {
        ((self.0 >> 4) & 0x3f) as u8
    }

    fn patch(&self) -> u8 {
        ((self.0) & 0xf) as u8
    }
}

impl std::fmt::Debug for Version {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}.{}.{}", self.major(), self.minor(), self.patch())?;
        Ok(())
    }
}

#[derive(Debug)]
struct Identify {
    version: Version,
    typ: u8,
    name: String,
}

impl Identify {
    fn parse(data: &[u8]) -> Option<Self> {
        if data.len() < 2 + 1 + 32 {
            return None;
        }

        let version = u16::from_le_bytes(data[0..2].try_into().ok()?);
        let typ = data[2];
        let name = &data[3..];

        let find_zero = |a: &[u8]| {
            for i in 0..a.len() {
                if a[i] == 0 {
                    return i;
                }
            }

            a.len()
        };

        let index = find_zero(name);
        let name = std::str::from_utf8(&name[0..index]).ok()?;

        Some(Identify {
            version: Version(version),
            typ,
            name: name.to_string(),
        })
    }
}

fn run(port: &String, baudrate: u32) {
    let mut port = serialport::new(port, baudrate)
        // .timeout(Duration::from_millis(5000))
        .open()
        .unwrap();

    // let stdin = std::io::stdin();
    // let mut lock = stdin.lock();
    // let mut s = String::new();
    // lock.read_line(&mut s).unwrap();
    // let s = s.trim();

    // let mut data = Vec::new();
    // data.push(0x01); // SET_DEVICE_CONTROLS
    // data.push(0x00); // device
    // data.push(0b000001111);
    // send_packet(&mut port, PacketType::Command, &data);
    //
    // let packet = wait_for_packet(&mut port);
    // println!("Packet: {:?}", packet);
    //
    // let mut data = Vec::new();
    // data.push(0x02); // GET_DEVICE_CONTROLS
    // data.push(0x00); // device
    // send_packet(&mut port, PacketType::Command, &data);
    //
    // let packet = wait_for_packet(&mut port);
    // println!("Packet: {:?}", packet);
    //
    // let mut data = Vec::new();
    // data.push(0x03); // GET_DEVICE_LINES
    // data.push(0x00); // device
    // send_packet(&mut port, PacketType::Command, &data);
    //
    // let packet = wait_for_packet(&mut port);
    // println!("Packet: {:?}", packet);

    let cmd = "identify";
    let cmd = parse_cmd(cmd).unwrap();
    match cmd {
        Command::Identify => {
            send_empty_packet(&mut port, PacketType::Identify);
            let packet = wait_for_packet(&mut port);
            let identity = Identify::parse(&packet.data[1..]);
            println!("Identity: {:?}", identity);
        }

        Command::SetDeviceControls { device, controls } => {
            let mut data = Vec::new();
            data.push(0x01); // SET_DEVICE_CONTROLS
            data.push(device); // device
            data.push(controls);
            send_packet(&mut port, PacketType::Command, &data);

            let packet = wait_for_packet(&mut port);
            println!("Packet: {:?}", packet);
        }

        Command::GetDeviceControls { device } => {
            let mut data = Vec::new();
            data.push(0x02); // GET_DEVICE_CONTROLS
            data.push(device); // device
            send_packet(&mut port, PacketType::Command, &data);

            let packet = wait_for_packet(&mut port);
            println!("Packet: {:?}", packet);
        }

        Command::GetDeviceLines { device } => {
            let mut data = Vec::new();
            data.push(0x03); // GET_DEVICE_LINES
            data.push(device); // device
            send_packet(&mut port, PacketType::Command, &data);

            let packet = wait_for_packet(&mut port);
            println!("Packet: {:?}", packet);
        }
    }
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
}
