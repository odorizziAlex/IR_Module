//==================== check Signal ====================
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <PriUint64.h>
const uint32_t kBaudRate = 115200;
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;
const uint16_t kFrequency = 38000;
IRrecv irrecv(RECV_PIN, kCaptureBufferSize, kTimeout, false);
decode_results results;
int codeType = -1;
int c = 0;
//======================================================
//======================== check protocol ==============================
void checkSignalInit(){
    Serial.begin(kBaudRate, SERIAL_8N1);
    irrecv.enableIRIn(); // Start up the IR receiver.
}

void checkType(decode_results *results)
{
    Serial.print("Protocol: ");
    codeType = results->decode_type;

    if (codeType == PANASONIC)
    {
        Serial.print("PANASONIC ");
    }
    else if (codeType == JVC)
    {
        Serial.print("JVC ");
    }
    else if (codeType == SONY)
    {
        Serial.print("SONY ");
    }
    else if (codeType == NEC)
    {
        Serial.print("NEC ");
    }
    else
    {
        Serial.print("UNKNOWN ");
    }
}

void checkProtocol(){
    if (irrecv.decode(&results))
    {
        Serial.print(c);
        Serial.print(" ");
        checkType(&results);
        Serial.println(PriUint64<HEX>(results.value));
        Serial.println();
        irrecv.resume();
        c++;
    }
}
//======================================================

