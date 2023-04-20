# RSNav Unit (Killer Queen)

## Status
* Byte 0
  * Bit 0 - Led Bar
  * Bit 1 - Led Bar (Low Mode)
  * Bit 2 - High Beam
  * Bit 3 - Led Bar Active
* Byte 1
  * Bit 0 - Reverse Camera
  * Bit 1 - Reverse Lights
  * Bit 2 - Reverse
  * Bit 3 - Reverse Lights Active
  * Bit 4 - Trunk Lights

## Commands
* 0x00 - SetLedBarActive
  * Parameter 0
    * Bit 0 - On/Off
    * Bit 1 - Toggle
* 0x01 - SetLedBarLowMode
  * Parameter 0
    * Bit 0 - On/Off
    * Bit 1 - Toggle
* 0x02 - ForceLedBar
  * Parameter 0
    * Bit 0 - On/Off
    * Bit 1 - Toggle
* 0x03 - SetTrunkLights
  * Parameter 0
    * Bit 0 - On/Off
    * Bit 1 - Toggle
* 0x04 - SetReverseLightsActive
  * Parameter 0
    * Bit 0 - On/Off
    * Bit 1 - Toggle
* 0x05 - ForceReverseLights
  * Parameter 0
    * Bit 0 - On/Off
    * Bit 1 - Toggle
* 0x06 - ForceReverseCamera
  * Parameter 0
    * Bit 0 - On/Off
    * Bit 1 - Toggle

# Front Unit (Crazy Diamond)

# Back Unit (Heaven's Door)