use enum_primitive_derive::Primitive;

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
pub enum JotaroCommands {
    Configure = 0x00,
    SetDeviceControls = 0x01,
    GetDeviceControls = 0x02,
    GetDeviceLines = 0x03,
}
