# CanBusProject

Create a custom Can Bus for a car

## Todos

- [ ] Create better docs for the Protocol

- [ ] Create a build system
  - [ ] Customize the firmware
  - [ ] Compile the firmware
- [ ] Firmware
  - [ ] Configuration
  - [ ] Inputs
  - [ ] Outputs
  - [ ] Can Bus
  - [ ] Communication
- [ ] Create schematic for the devices
- [ ] Functions on the canbus instead of controlling controls directly

## Devices

### Star Platinum (SP)

- Control unit for the RSNav
- Communicate through COM-Port
- Connection to Can bus

#### Commands

- CONFIGURE

### Gold Experience (GE)

- Can bus device

### Commands

## Software

### The World (TW)

- Firmware for devices
- Customizable with the build system (W.I.P)

### DIO

- Interact with devices via a custom protocol

## Notes

- https://boltrobotics.com/2018/01/09/Custom-Protocol-for-Serial-Link-Communication/

- https://probe.rs/

arduino-cli compile -b arduino:avr:nano --build-property 'build.extra_flags="-DTEST=123"' -u -p COM4 -b arduino:avr:nano

openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -s tcl -c "adapter speed 5000"
