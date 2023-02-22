void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    delay(1000);
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
