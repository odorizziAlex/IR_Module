#include <Arduino.h>
#include <Config.h>
#include <transmitter.h>

const int IRLEDpin = D5;

//======================== protocol/ technical variables ========================
unsigned int frequency;
unsigned int wavePeriodDuration;
unsigned int leadPulseLength;
unsigned int leadPauseLength;
unsigned int stopPulseLength;
unsigned int BitTime;
unsigned int writeHIGHExectionTime = 0.96;
unsigned int writeLOWExectionTime = 1.01;
unsigned int HIGHBitTime;
//======================== message variables ========================
unsigned int msgReferenceBitCode;
int msgBitNumber;
unsigned int cmdReferenceBitCode;
int cmdBitNumber;

int totalChunkNumber = -1;
unsigned int codes[255];
int chunkIndex = 0;

void initTransmitter(unsigned int freq, unsigned int wavePrdD,
                     unsigned int lPulseLen, unsigned int lPauseLen, unsigned int sPulseLen,
                     unsigned int lowBTime, unsigned int highBTime,
                     const unsigned int msgBitNum, const unsigned int msgReferenceBitSeq,
                     const unsigned int cmdBitNum, const unsigned int cmdReferenceBitSeq)
{
    msgBitNumber = msgBitNum;
    msgReferenceBitCode = msgReferenceBitSeq;
    cmdBitNumber = cmdBitNum;
    cmdReferenceBitCode = cmdReferenceBitSeq;
    frequency = freq;
    wavePeriodDuration = wavePrdD;
    leadPulseLength = lPulseLen;
    leadPauseLength = lPauseLen;
    stopPulseLength = sPulseLen;
    BitTime = lowBTime;
    HIGHBitTime = highBTime;
}

// This function makes the IR-LED oscillate on the needed carrier frequency
void IRcarrier(unsigned int IRTimeMicros)
{
    for (unsigned int i = 0; i < (IRTimeMicros / wavePeriodDuration); i++)
    {
        digitalWrite(IRLEDpin, HIGH);
        // this delay also takes execution time of digitalWrite HIGH in consideration
        delayMicroseconds((wavePeriodDuration / 2) - writeHIGHExectionTime);
        digitalWrite(IRLEDpin, LOW);
        // this delay also takes execution time of digitalWrite LOW in consideration
        delayMicroseconds((wavePeriodDuration / 2) - writeLOWExectionTime);
    }
}

// This function actually sends codes bit by bit.
// It needs the bitcode itself, the number of bits, the code consists of and a reference bit
// Code to iterate bitwise
void sendIRCode(unsigned int code, unsigned int bitNumber, unsigned int referenceCode)
{
    // Send the leading pulse and wait for lead pause duration
    IRcarrier(leadPulseLength);
    delayMicroseconds(leadPauseLength);

    // Sends code sequence bit by bit using bitshifts
    for (unsigned int i = 0; i < bitNumber; i++)
    {
        // Get the current bit by masking all but the MSB
        if (code & referenceCode)
            // Turn on the carrier for one bit time
            IRcarrier(HIGHBitTime);
        else
            // Turn on the carrier for zero bit time
            IRcarrier(BitTime);

        delayMicroseconds(BitTime);
        // Shift to the next bit
        code <<= 1;
    }
    // Send a single STOP bit.
    IRcarrier(stopPulseLength);

    // Update timeout timer
    updateResponseTimer();
}

// Loads sequence of chunks and sets number of chunks
void loadIRCodeSequence(unsigned int chunkNum, unsigned int *code)
{
    totalChunkNumber = chunkNum;
    for (int i = 0; i < totalChunkNumber; i++)
        codes[i] = code[i];
}

void resetTransmitter()
{
    chunkIndex = 0;
    for (int i = 0; i < totalChunkNumber; i++)
        codes[i] = 0;
    totalChunkNumber = -1;
}

void updateCodeChunkIndexTransm(unsigned int n)
{
    chunkIndex += n;
    // This will only be called, if the transmitting-module-chunk-index is synchronous
    // with the receiving-Module-chunk-index.
    // In case both indexes are not synchronous, a stop signal will be sent from the reveiving
    // module, which stops further transmission
    if (chunkIndex > totalChunkNumber - 1)
    {
        //TESTING
        updateMeasurementMain();
        moduleResponseToHost();
        return;
    }
    sendIRCode(codes[chunkIndex], msgBitNumber, msgReferenceBitCode);
}