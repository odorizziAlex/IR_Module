#include <Arduino.h>
#include <receiver.h>
#include <Config.h>
#include <stdio.h>
#include <stdlib.h>

enum recvState
{
    WAITING,
    LISTENING
};

//TESTING
boolean calibrateRecv;

int superStateRecv = 0;

double startBurstDurationInterval[2];
double startPauseDurationInterval[2];
double endBurstDurationInterval[2];
double oneBurstDurationInterval[2];
double zeroBurstDurationInterval[2];

const int msgBitNumber = 32;
const int cmdBitNumber = 8;
const int startChunkBitNumber = 16;
const int chunkIdBitNumber = 2;
const int tolerableMistakeNumber = 1;
const unsigned int maxChunkNumber = 255;

int chunkNumber = -1;

// Duration of one maximum Signal (0xFFFFFFFF): (after start burst + end pause) 32 * ("1" + pause) + end burst
int maximumTimeoutDuration; // maximum siganl duration: 65,82 milliseconds

int sequences[maxChunkNumber][msgBitNumber];
unsigned int finalSequences[maxChunkNumber];

int codeChunkIndex = 0;
int previousChunkId = -1;
int bitIterator = 0;
int errorCounter = 0;

int recvState = WAITING;
unsigned long startReceiverTimestamp;
unsigned long startBurstTimer;
unsigned long calibrationTimer;
// unsigned long burstDuration;
// unsigned long pauseDuration;
boolean startSignal = false;
boolean transmissionInitialized = false;
boolean toggle = false;
boolean isSignalPassed = false;

//TESTING
unsigned int logCounter = 0;

void resetSequenceData()
{
    for (int i = 0; i < msgBitNumber; i++)
        sequences[codeChunkIndex][i] = -1;
    bitIterator = 0;
    errorCounter = 0;
}

void initReceiver(boolean calibrate, const double *startBrstDInterval, const double *startPsDInterval,
                  const double *endBrstDInterval, const double *oneBrstDInterval,
                  const double *zeroBrstDInterval, const unsigned int timeoutThreshold)
{
    calibrateRecv = calibrate;
    startBurstDurationInterval[0] = startBrstDInterval[0];
    startBurstDurationInterval[1] = startBrstDInterval[1];
    startPauseDurationInterval[0] = startPsDInterval[0];
    startPauseDurationInterval[1] = startPsDInterval[1];
    endBurstDurationInterval[0] = endBrstDInterval[0];
    endBurstDurationInterval[1] = endBrstDInterval[1];
    oneBurstDurationInterval[0] = oneBrstDInterval[0];
    oneBurstDurationInterval[1] = oneBrstDInterval[1];
    zeroBurstDurationInterval[0] = zeroBrstDInterval[0];
    zeroBurstDurationInterval[1] = zeroBrstDInterval[1];
    maximumTimeoutDuration = timeoutThreshold;
    //?
    resetSequenceData();
}

void resetReceiver()
{
    while (codeChunkIndex > -1)
    {
        resetSequenceData();
        finalSequences[codeChunkIndex] = 0;
        codeChunkIndex--;
    }
    codeChunkIndex = 0;
    previousChunkId = -1;
    chunkNumber = -1;
    recvState = WAITING;
    startReceiverTimestamp = 0;
    startBurstTimer = 0;
    calibrationTimer = 0;
    // burstDuration = 0;
    // pauseDuration = 0;
    startSignal = false;
    transmissionInitialized = false;
    toggle = false;
    isSignalPassed = false;
}

void updateSuperStateRecv(int state)
{
    superStateRecv = state;
}

unsigned int decodeSequence(int *seq, int bitNumber)
{
    unsigned int chunk = 0;
    for (int i = 0; i < bitNumber; i++)
    {
        if (seq[i] == 1)
            chunk++;
        if (i < bitNumber - 1)
            chunk <<= 1;
    }
    return chunk;
}

unsigned int verifyChunkId()
{
    // Perform error correction if needed.
    if(!inspectChunkContent(sequences[codeChunkIndex]))
        return REPEAT;
        
    // Extract Id bits at index 3 and 5 and decode to integer
    int idBits[] = {sequences[codeChunkIndex][3], sequences[codeChunkIndex][5]};
    int currentChunkId = decodeSequence(idBits, chunkIdBitNumber);

    if (currentChunkId == previousChunkId)
        return NEXT_PKT;
    else if (previousChunkId == -1)
        previousChunkId = currentChunkId;
    else if (currentChunkId == previousChunkId + 1 && previousChunkId != 3)
        previousChunkId = currentChunkId;
    else if (previousChunkId == 3 && currentChunkId == 0)
        previousChunkId = currentChunkId;
    else
        return REPEAT;

    unsigned int numericValueFinal = decodeSequence(sequences[codeChunkIndex], msgBitNumber);

    finalSequences[codeChunkIndex] = numericValueFinal;
    return VALID_MESSAGE;
}

unsigned int checkIfCommandIncomming()
{
    unsigned int incomingSequence;
    if (bitIterator == cmdBitNumber)
    {
        incomingSequence = decodeSequence(sequences[codeChunkIndex], cmdBitNumber);

        if (incomingSequence == STOP)
            return STOP;
        else if (incomingSequence == START)
            return START;
        else if (incomingSequence == NEXT_PKT)
            return NEXT_PKT;
        else if (incomingSequence == REPEAT)
            return REPEAT;
    }
    else if (bitIterator == msgBitNumber)
        return VALID_MESSAGE;
    else if (bitIterator == startChunkBitNumber)
        return CHUNK_NUMBER;

    return CMD_NOT_FOUND_ERROR;
}

void initTransmission(unsigned int incomingSequence)
{
    if (incomingSequence == REPEAT)
    {
        if (superStateRecv == NONE)
        {
            sendResponse(REPEAT);
            updateSuperStateMain(RECEIVER);
        }
        else if (superStateRecv == RECEIVER)
            sendResponse(REPEAT);
        else if (superStateRecv == SENDER)
            sendResponse(CHUNK_NUMBER);
        resetSequenceData();
    }
    else if (incomingSequence == START)
    {
        if (superStateRecv == SENDER)
        {
            transmissionInitialized = true;
            setTransmissionInitializedMain(transmissionInitialized);
            updateCodeChunkIndexMain(0);
        }
        resetSequenceData();
    }
    else if (incomingSequence == CHUNK_NUMBER)
    {
        unsigned int incomingChunkNumber = decodeSequence(sequences[codeChunkIndex], startChunkBitNumber);
        if (incomingChunkNumber <= maxChunkNumber)
        {
            //TESTING
            updateTransmissionTimer();
            
            chunkNumber = incomingChunkNumber;
            updateSuperStateMain(RECEIVER);
            transmissionInitialized = true;
            setTransmissionInitializedMain(transmissionInitialized);
            sendResponse(START);
            resetSequenceData();
        }
        else
            resetSequenceData();
    }
}

void receiveData(unsigned int incomingSequence)
{
    if (incomingSequence == STOP)
    {
        if (superStateRecv == SENDER)
            moduleResponseToHost();
        //TESTING
        updateMeasurementMain();
    }
    else if (incomingSequence == NEXT_PKT)
    {
        if (superStateRecv == SENDER)
        {
            updateCodeChunkIndexMain(1);
            resetSequenceData();
            updateSuccessIndicator();
        }
    }
    else if (incomingSequence == REPEAT)
    {
        if (superStateRecv == SENDER)
            updateCodeChunkIndexMain(0);
        else if (superStateRecv == RECEIVER || superStateRecv == NONE)
            sendResponse(REPEAT);
        resetSequenceData();
    }
    else if (incomingSequence == VALID_MESSAGE)
    {
        unsigned int response = verifyChunkId();
        if (response == VALID_MESSAGE)
        {
            codeChunkIndex++;
            if(codeChunkIndex == chunkNumber)
                sendResponse(STOP);
            else
                sendResponse(NEXT_PKT);
            
            resetSequenceData();
            updateSuccessIndicator();
        }
        else
        {
            sendResponse(response);
            resetSequenceData();
        }
    }
}

void analyzeSequence()
{
    unsigned int incomingSequence = checkIfCommandIncomming();
    loggerMain(incomingSequence);
    if (incomingSequence == CMD_NOT_FOUND_ERROR)
    {
        sendResponse(REPEAT);
        resetSequenceData();
    }
    else if (!transmissionInitialized)
        initTransmission(incomingSequence);
    else
        receiveData(incomingSequence);
}

// This function differentiates between legal signals and illegal signals using the received bit number, stored in bitIterator.
// Start signal = 16 Bit; Command signal = 8 Bit; Content signal = 32 Bit
unsigned int inspectSignal()
{
    recvState = WAITING;
    // Return repeat request if signal has too many bit errors.
    if (errorCounter > tolerableMistakeNumber)
        return REPEAT;
    // Return Stop command if sender hasn't received transmission terminator
    else if (!transmissionInitialized && bitIterator == msgBitNumber)
        return STOP;
    // Return repeat request if transmission is not initialized and not a command or start chunk is received.
    else if (!transmissionInitialized && bitIterator != startChunkBitNumber && bitIterator != cmdBitNumber)
        return REPEAT;
    // Return start command if transmission is initialized, no content chunks are received yet and a start chunk sized Signal is received.
    else if (transmissionInitialized && codeChunkIndex == 0 && bitIterator == startChunkBitNumber)
        return START;
    // Return repeat request, if the received bit number matches neither start chunk bit Number, command bit number or content bit number.
    else if (transmissionInitialized && bitIterator != cmdBitNumber && bitIterator != msgBitNumber)
        return REPEAT;
    return VALID_MESSAGE;
}

void evaluateSignal(int burstDuration)
{
    if (burstDuration >= oneBurstDurationInterval[0] && burstDuration <= oneBurstDurationInterval[1])
    {
        sequences[codeChunkIndex][bitIterator] = 1;
        bitIterator++;
    }
    else if (burstDuration >= zeroBurstDurationInterval[0] && burstDuration <= zeroBurstDurationInterval[1])
    {
        sequences[codeChunkIndex][bitIterator] = 0;
        bitIterator++;
    }
    else if (bitIterator <= msgBitNumber && burstDuration >= endBurstDurationInterval[0] && burstDuration <= endBurstDurationInterval[1])
    {
        unsigned int command = inspectSignal();
        if (command == VALID_MESSAGE)
            analyzeSequence();
        else
        {
            sendResponse(command);
            resetSequenceData();
        }
    }
    else
    {
        sequences[codeChunkIndex][bitIterator] = 0;
        if (transmissionInitialized)
            bitIterator++;
        errorCounter++;
    }
}

void identifyStartBurst(int burstDuration)
{
    if (recvState == WAITING && burstDuration >= startBurstDurationInterval[0] && burstDuration <= startBurstDurationInterval[1])
    {
        startSignal = true;
        startBurstTimer = micros();
    }
}

void identifyStartPause()
{
    if (startSignal == true)
    {
        int pauseDuration = micros() - startBurstTimer;
        //TESTING
        if (calibrateRecv)
        {
            Serial.print(pauseDuration);
            Serial.print(";");
        }

        if (pauseDuration >= startPauseDurationInterval[0] && pauseDuration <= startPauseDurationInterval[1])
        {
            recvState = LISTENING;
            startReceiverTimestamp = micros();
            startSignal = false;
        }
    }
}

// Sends repeat signal once when no signal is detected in the expected time frame to
// speed up the retransmission, instead of waiting for sender timeout
void checkSequenceTimeout()
{
    if (recvState == LISTENING && micros() >= startReceiverTimestamp + maximumTimeoutDuration)
    {
        //TESTING
        loggerMain(187);
        recvState = WAITING;
        resetSequenceData();
        sendResponse(REPEAT);
    }
}

// Passes received data to serial for further analyzation
void processData()
{
    isSignalPassed = true;
    boolean dataReceived = true;
    //TESTING
    updateMeasurementMain();
    fillBufferInMain(chunkNumber, finalSequences, dataReceived);
    moduleResponseToHost();
}

void receiveIRCode(int incomingBit)
{
    if (codeChunkIndex == chunkNumber && chunkNumber != -1 && !isSignalPassed)
        processData();
    else
    {
        // Save timestamp of the first time, a signal is detected
        if (incomingBit == 0 && toggle == false)
        {
            toggle = true;
            /**
             * When IR burst of specified start length is received, check length of the bursts following pause.
             * When both durations, (burst and pause) are within their specified intervals, only then start listening
             * for following bits.
             */
            identifyStartPause();
            startBurstTimer = micros();
        }
        // Evaluate Burst Duration and identify information
        else if (incomingBit == 1 && toggle == true)
        {
            toggle = false;
            int burstDuration = micros() - startBurstTimer;
            //TESTING
            if (calibrateRecv)
            {
                Serial.print(burstDuration);
                Serial.print(";");
            }
            identifyStartBurst(burstDuration);
            if (recvState == LISTENING)
                evaluateSignal(burstDuration);
        }
        checkSequenceTimeout();
    }
}
