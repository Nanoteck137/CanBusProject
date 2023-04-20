# Protocol (Speedwagon)

## Base Packet

| ITEM         | OFFSET       | LENGTH   |
| ------------ | ------------ | -------- |
| PACKET_START | 0            | 1        |
| PID          | 1            | 1        |
| TYP          | 2            | 1        |
| DATA_LEN     | 3            | 1        |
| DATA         | 4            | DATA_LEN |
| CHECKSUM     | DATA_LEN + 4 | 2        |

### TYP: Identify 0x00 (DATA)
* No DATA

### TYP: Status 0x01 (DATA)
* No DATA

### TYP: Command 0x02 (DATA)
| ITEM       | OFFSET | LENGTH       |
| ---------- | ------ | ------------ |
| CMD_INDEX  | 0      | 1            |
| CMD_PARAMS | 1      | DATA_LEN - 1 |

### TYP: Ping 0x03 (DATA)
* No DATA

### TYP: Update 0x04 (DATA)
* No DATA

### TYP: Response 0x05 (DATA)
* [Packet Response](#packet-response)

## Packet Response

| ITEM          | OFFSET | LENGTH       |
| ------------- | ------ | ------------ |
| ERROR_CODE    | 0      | 1            |
| RESPONSE_DATA | 1      | DATA_LEN - 1 |

### Identity (RESPONSE_DATA)

| ITEM         | OFFSET       | LENGTH   |
| ------------ | ------------ | -------- |
| VERSION      | 0            | 2        |
| NUM_CMDS     | 2            | 1        |
| NAME_LEN     | 3            | 1        |
| NAME         | 3            | NAME_LEN | 

### Status (RESPONSE_DATA)

| ITEM          | OFFSET       | LENGTH   |
| ------------- | ------------ | -------- |
| STATUS_BUFFER | 0            | 16       |

### Command (RESPONSE_DATA)
* Has no RESPONSE_DATA 

### Ping (RESPONSE_DATA)
* Has no RESPONSE_DATA 