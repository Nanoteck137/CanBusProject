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
// DATA LEN (1-Byte)
// DATA (26 Max)
// Checksum (2-bytes)

const uint8_t START = 0x4e;

const uint8_t SYN = 0x01;
const uint8_t SYN_ACK = 0x02;
const uint8_t ACK = 0x03;
const uint8_t PING = 0x04;
const uint8_t PONG = 0x05;
const uint8_t UPDATE = 0x06;

const int SUCCESS = 0;
const int UNKNOWN_ERROR = 0;

struct Packet {
    uint8_t type;
    uint8_t pid;
    size_t data_length;
    uint8_t *data;
    uint16_t checksum;
};

uint8_t pid = 0;

bool connection = false;
bool got_syn = false;

bool sent_ping = false;
bool waiting_for_pong = false;

unsigned long last = 0;

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

void send_packet(uint8_t type, uint8_t *data, uint8_t len) {
    if(!data)
        len = 0;

    uint8_t *start = buf_ptr;

    // Start byte
    write_u8(START);

    uint8_t *checksum_start = buf_ptr;

    // PID
    write_u8(pid++);

    // TYPE
    write_u8(type);

    // DATA LENGTH
    write_u8(len);

    // DATA
    for(size_t i = 0; i < len; i++) {
        write_u8(data[i]);
    }

    size_t checksum_len = (size_t)(buf_ptr - start);
    uint16_t checksum = tusk_checksum(checksum_start, checksum_len);

    // CHECKSUM
    write_u16(checksum);

    size_t packet_len = (size_t)(buf_ptr - start);
    write_buffer(packet_len);
}

void send_empty_packet(uint8_t type) {
    send_packet(type, nullptr, 0);
}

int parse_packet(Packet *packet) {
    while(true) {
        // Read the header of the packet
        if(Serial.available() >= 3) {
            uint8_t temp[3];
            Serial.readBytes(temp, sizeof(temp));

            packet->pid = temp[0];
            packet->type = temp[1];
            packet->data_length = temp[2];

            for(int i = 0; i < packet->data_length; i++) {
                Serial.read();
            }

            while(true) {
                if(Serial.available() >= 2) {
                    Serial.readBytes(temp, sizeof(temp));

                    uint16_t a = (uint16_t)temp[0];
                    uint16_t b = (uint16_t)temp[1];
                    packet->checksum = b << 8 | a;

                    break;
                }
            }

            return SUCCESS;
        }
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    last = millis();
}


void loop() {
    // Get connection
    if(Serial.available() > 0 && !connection) {
        int b = Serial.read();
        if(b == START) {
            Packet packet;
            if(parse_packet(&packet) == SUCCESS) {
                if(!got_syn && packet.type == SYN) {
                    send_empty_packet(SYN_ACK);
                    got_syn = true;
                }

                if(got_syn && packet.type == ACK) {
                    connection = true;
                }
            }
        }
    }

    if(connection) {
        if(!sent_ping) {
            send_empty_packet(PING);
            sent_ping = true;
            waiting_for_pong = true;
        }

        if(Serial.available() > 0) {
            int b = Serial.read();
            if(b == START) {
                Packet packet;
                if(parse_packet(&packet) == SUCCESS) {
                    if(packet.type == PONG) {
                        waiting_for_pong = false;
                    }
                }
            }
        }

        if((millis() - last) > 2000) {
            uint8_t data[] = { 123 };
            send_packet(UPDATE, data, sizeof(data));
            last = millis();
        }
    }


}
