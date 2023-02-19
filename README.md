# CanBusProject

Create a custom Can Bus for a car

## Devices

### Star Platinum (SP)
- Control unit for the RSNav 
- Communicate through COM-Port 

### The World (TW)
- Firmware for Star Platinum
- https://boltrobotics.com/2018/01/09/Custom-Protocol-for-Serial-Link-Communication/
- Serial Communication
    - Protocol:
    - Header (1 Byte)
    - Length (1 Byte)
    - Fletcher16 (2 Bytes)

### Gold Experience (GE) 
- Can bus device
- 2 Inputs (+12v)
- 2 Outputs (+12v Relayed)

### King Crimson (KC)
- Firmware for Gold Experience
- Every device should have same firmware but diffrent Can-Ids so we need to build a system to automate the creation of the firmware for every device

### DIO 
- Interact with the serial port for The World firmware protocol 

arduino-cli compile -b arduino:avr:nano --build-property 'build.extra_flags="-DTEST=123"' -u -p COM4 -b arduino:avr:nano