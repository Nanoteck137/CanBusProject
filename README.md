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
  - [ ] Can Bus Messages
- [ ] Create schematic for the devices

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

arduino-cli compile -b arduino:avr:nano --build-property 'build.extra_flags="-DTEST=123"' -u -p COM4 -b arduino:avr:nano