#include <tusk.h>

uint8_t buf[32] = { 0 };

uint8_t *buf_ptr = buf;

// Handshake:
// SYN
// SYN-ACK
// ACK

// Packet:
// START (1-Byte)
// PID (1-Byte)
// TYPE (1-Byte)
// DATA (27 Max)
// Checksum (2-bytes)

const uint8_t START = 0x4e;

const uint8_t SYN = 0x01;
const uint8_t SYN_ACK = 0x02;
const uint8_t ACK = 0x03;
const uint8_t PING = 0x04;
const uint8_t PONG = 0x05;

void write_u8(uint8_t val) {
    *buf_ptr++ = val;
}

void write_u16(uint16_t val) {
    *buf_ptr++ = val & 0xff;
    *buf_ptr++ = (val >> 8) & 0xff;
}

void write_buffer(size_t length) {
    Serial.write(buf, length);
    Serial.flush();

    buf_ptr = buf;
}

void send_syn() {
    uint8_t *start = buf_ptr;

    // Start byte
    write_u8(START);
    // PID
    write_u8(0);
    // TYPE
    write_u8(SYN);
    size_t len = (size_t)(buf_ptr - start);
    uint16_t checksum = tusk_checksum(start, len);
    write_u16(checksum);

    size_t packet_len = (size_t)(buf_ptr - start);
    write_buffer(packet_len);
}

void send_ack() {
    uint8_t *start = buf_ptr;

    // Start byte
    write_u8(START);
    // PID
    write_u8(0);
    // TYPE
    write_u8(ACK);
    size_t len = (size_t)(buf_ptr - start);
    uint16_t checksum = tusk_checksum(start, len);
    write_u16(checksum);

    size_t packet_len = (size_t)(buf_ptr - start);
    write_buffer(packet_len);
}

void send_pong() {
    uint8_t *start = buf_ptr;

    // Start byte
    write_u8(START);
    // PID
    write_u8(0);
    // TYPE
    write_u8(PONG);
    size_t len = (size_t)(buf_ptr - start);
    uint16_t checksum = tusk_checksum(start, len);
    write_u16(checksum);

    size_t packet_len = (size_t)(buf_ptr - start);
    write_buffer(packet_len);
}

int parse_packet() {
    int typ = -1;
    while(true) {
        // Read the header of the packet
        if(Serial.available() >= 2) {
            uint8_t temp_buf[2];
            Serial.readBytes(temp_buf, sizeof(temp_buf));

            uint8_t pid = temp_buf[0];
            typ = (int)temp_buf[1];

            while(true) {
                if(Serial.available() >= 2) {
                    Serial.readBytes(temp_buf, sizeof(temp_buf));
                    break;
                }
            }
        }

        if(typ != -1) {
            break;
        }
    }

    return typ;
}

void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    while(!Serial);

    send_syn();

    bool connection = false;

    while(!connection) {
        if(Serial.available() > 0) {
            int r = Serial.read();
            if(r == START) {
                int e = parse_packet();
                if(e == SYN_ACK) {
                    send_ack();
                    connection = true;
                }
            }
        }
    }

    while(connection) {
        if(Serial.available() > 0) {
            int r = Serial.read();
            if(r == START) {
                int e = parse_packet();
                if(e == PING) {
                    send_pong();
                }
            }
        }
    }

    while(true);

    // uint8_t data[] = { 1, 2, 0, 3, 4 };
    // size_t buf_len = tusk_get_max_encode_buffer_size(sizeof(data));
    // size_t len = tusk_encode(data, sizeof(data), buf, 0);
    // Serial.write(buf, len);
    // Serial.flush();
    //
    // delay(10000);
    //
    // bool found = false;
    // uint8_t *ptr = buf;
    //
    // while(Serial.available() <= 0 || !found) {
    //     int r = Serial.read();
    //     if(r == 4) {
    //         found = true;
    //     }
    //
    //     if(found) {
    //         *ptr++ = r;
    //     }
    // }
    //
    // size_t rlen = (size_t)(ptr - buf);
    // Serial.write((uint8_t *)&rlen, sizeof(rlen));
    // Serial.flush();
}

enum Commands
{
    CMD_GET,
    CMD_SET,
    NUM_CMDS
};

const int TRANSMISSION_TIMEOUT = 2000;

bool transmissionStarted = false;

int currentCmd = 0x00;
int cmdDataSize = 0;

int buffer[0x10] = {0};
int ptr = 0;

int getCmdDataSize()
{
    switch (currentCmd)
    {
    case CMD_GET:
        return 1;
    case CMD_SET:
        return 1 + sizeof(uint16_t);
    default:
        Serial.print("Unknown Command: ");
        Serial.println(currentCmd, HEX);
    }
}

unsigned long last = 0;

void StartTransmission()
{
    transmissionStarted = true;
    last = millis();
    ptr = 0;
}

void StopTransmission()
{
    transmissionStarted = false;
}

bool first = true;

void loop()
{
    if(Serial.available()) {
        int read = Serial.read();

        if(first) {
            Serial.println("This is first");
            first = false;
        } else {
            Serial.println("Test");
        }
    }

    // Serial.write(writes);
    // writes++;
    // delay(1000);

    // if (!transmissionStarted)
    // {
    //     if (Serial.available() > 0)
    //     {
    //         currentCmd = Serial.read();
    //         Serial.print("Started cmd: ");
    //         Serial.println(currentCmd, HEX);
    //
    //         cmdDataSize = getCmdDataSize();
    //         Serial.print("Expected Data Size: ");
    //         Serial.println(cmdDataSize, HEX);
    //
    //         StartTransmission();
    //     }
    // }
    //
    // if (transmissionStarted)
    // {
    //     for (int i = 0; i < Serial.available() && ptr < cmdDataSize; i++)
    //     {
    //         buffer[ptr] = Serial.read();
    //         ptr++;
    //     }
    //
    //     if (ptr == cmdDataSize)
    //     {
    //         Serial.println("All?");
    //         for (int i = 0; i < ptr; i++)
    //         {
    //             Serial.print(buffer[i], HEX);
    //             Serial.print(" ");
    //         }
    //         Serial.println();
    //
    //         if (currentCmd == CMD_SET)
    //         {
    //             Serial.println("Light");
    //             digitalWrite(LED_BUILTIN, buffer[0] == 0 ? LOW : HIGH);
    //         }
    //
    //         StopTransmission();
    //     }
    // }
    //
    // if (transmissionStarted)
    // {
    //     unsigned long current = millis();
    //     if (current - last > TRANSMISSION_TIMEOUT)
    //     {
    //         Serial.println("Timeout");
    //         uint8_t bytes[] = {123, 321};
    //         Serial.write(bytes, sizeof(bytes));
    //         StopTransmission();
    //     }
    // }
}
