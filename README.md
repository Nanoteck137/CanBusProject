# CanBusProject

Create a custom Can Bus for a car

## Todos

- [ ] Create a build system
  - [ ] Compile the firmware
  - [ ] Customize the firmware
- [ ] Firmware
  - [ ] Configuration
  - [ ] Inputs
  - [ ] Outputs
  - [ ] Can Bus 
  - [ ] Communication
- [ ] Create schematic for the devices

## Protocol

### Packet Structure
Start (1-Byte)
Packet Type (1-Byte)
Data Length (1-Byte)
Data (Data Length)
Checksum (2-Byte, NOT USED YET)

### Packet Types
- IDENTIFY
  - No data
- COMMAND
  - Command (1-byte)
  - Command Body
- PING
  - No data
- UPDATE
  - Update Data
  - No response
- RES
  - Response Code (1-Byte)
  - Reposnse Body
    - For success this can be data we need to return
    - For errors the byte after the response code is the error code

### Response Codes
- Success (0x00)
- Error (0x01)

### Commands
- SET
- GET
- CONFIGURE
  - Send Updates (bool/1-Byte) 0/off >1/on

## Devices

### Star Platinum (SP)

- Control unit for the RSNav
- Communicate through COM-Port

### The World (TW)

- Firmware for Star Platinum
- Serial Communication Protocol
  - Packet
    - Start Byte (1-Byte)
    - Packet ID (1-Byte)
    - Type (1-Byte)
    - Data Length (1-Byte)
    - Data (26-Bytes Max)
    - Checksum (2-bytes)
  - Types
    - SYN (0x01)
    - SYN_ACK (0x02)
    - ACK (0x03)
    - PING (0x04)
    - PONG (0x05) (Replace with RESULT type)
    - UPDATE (0x06)

### Gold Experience (GE)

- Can bus device
- 2 Inputs (+12v)
- 2 Outputs (+12v Relayed)

### King Crimson (KC)

- Firmware for Gold Experience
- Every device should have same firmware but diffrent Can-Ids so we need to build a system to automate the creation of the firmware for every device

### DIO

- Interact with the serial port for The World firmware protocol

## Notes

- https://boltrobotics.com/2018/01/09/Custom-Protocol-for-Serial-Link-Communication/

- https://probe.rs/

arduino-cli compile -b arduino:avr:nano --build-property 'build.extra_flags="-DTEST=123"' -u -p COM4 -b arduino:avr:nano

openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -s tcl -c "adapter speed 5000"