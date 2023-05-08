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
2 - ERROR

3 - CMD
4 - IDENTIFY
5 - STATUS

6 - ON_CONNECT
7 - ON_CMD
8 - ON_IDENTIFY
9 - ON_STATUS


