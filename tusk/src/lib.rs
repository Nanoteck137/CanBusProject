use enum_primitive_derive::Primitive;

pub const MAX_DEVICES: usize = 8;
pub const MAX_IO: usize = 8;

pub const PACKET_START: u8 = 0x4e;

#[derive(Copy, Clone, Primitive, PartialEq, Debug)]
#[repr(u8)]
pub enum PacketType {
    Identify = 0x00,
    Status = 0x01,
    ExecFunc = 0x02,
    Ping = 0x03,
    Update = 0x04,
    Response = 0x05,
}

#[derive(Copy, Clone, Primitive, PartialEq, Debug)]
#[repr(u8)]
pub enum ResponseType {
    Success = 0x00,
    Err = 0x01,
}

#[derive(Copy, Clone, Primitive, PartialEq, Debug)]
#[repr(u8)]
pub enum ErrorCode {
    Success = 0x00,
    Unknown = 0x01,
    InvalidPacketType = 0x02,
    InvalidCommand = 0x03,
    InvalidResponse = 0x04,
    InvalidDevice = 0x05,
    InsufficientFunctionParameters = 0x06,
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
