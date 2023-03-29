use enum_primitive_derive::Primitive;

pub const MAX_DEVICES: usize = 8;
pub const MAX_IO: usize = 8;

pub const PACKET_START: u8 = 0x4e;

#[derive(Copy, Clone, Primitive, PartialEq, Debug)]
#[repr(u8)]
pub enum PacketType {
    Identify = 0x00,
    Command = 0x01,
    Ping = 0x02,
    Update = 0x03,
    Response = 0x04,
}

#[derive(Copy, Clone, Primitive, PartialEq, Debug)]
#[repr(u8)]
pub enum ResponseType {
    Success = 0x00,
    Err = 0x01,
}

/// Device Types
#[derive(Copy, Clone, Primitive, PartialEq, Debug)]
#[repr(u8)]
pub enum DeviceType {
    StarPlatinum = 0x00,
    GoldExperience = 0x01,
}

/// Commands for Star Platinum (SP) Devices
#[derive(Copy, Clone, Primitive, PartialEq, Debug)]
#[repr(u8)]
pub enum SPCommands {
    Configure = 0x00,
    SetDeviceControls = 0x01,
    GetDeviceControls = 0x02,
    GetDeviceLines = 0x03,
}

/// Commands for Gold Experience (GE) Devices
#[derive(Copy, Clone, Primitive, PartialEq, Debug)]
#[repr(u8)]
pub enum GECommands {
    Empty = 0x00,
}
