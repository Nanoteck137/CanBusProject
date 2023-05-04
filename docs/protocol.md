# Protocol (Speedwagon)

## Base Packet

| ITEM         | OFFSET       | LENGTH |
| ------------ | ------------ | ------ |
| TYP          | 0            | 1      |
| ID           | 1            | 2      | (Little Endian)
| DATA         | 4            | VAR    |

Packet Max Size: 259

0 - CONNECT
1 - DISCONNECT

2 - CMD
3 - IDENTIFY
4 - STATUS

5 - RESPONSE
