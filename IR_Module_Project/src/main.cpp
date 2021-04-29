#include <Arduino.h>
#include <receiver.h>
#include <transmitter.h>
#include <dataPrep.h>
#include <Config.h>

const int RECV_PIN = D1;
const int IR_LED_PIN = D5;
const int BTN_PIN = D7;
const int FEEDBACK_LED = D3;

//TESTING
boolean calibrate = false;
//TESTING
boolean testing = true;

boolean moduleIsConnected = false;
SuperState superState = NONE;
TransmissionState transmitPermission = WAITING_FOR_PERMISSION;

const unsigned int frequency = 57.6;
const unsigned int wavePeriodDuration = 1000 / frequency;
// ===================== START Modulation Scheme durations =====================
const unsigned int leadPulseLength = 9000;
const unsigned int leadPauseLength = 1500;
const unsigned int stopPulseLength = 1000;
const unsigned int BitTime = 560;
const unsigned int HIGHBitTime = 3 * BitTime;
// ===================== END Modulation Scheme durations =====================
// ===================== START Receiver Variables =====================
const double startBurstDurationInterval[2] = {10260.00, 10730.00};
const double startPauseDurationInterval[2] = {1390.00, 1643.05};
const double endBurstDurationInterval[2] = {1074.00, 1180.00};
const double oneBurstDurationInterval[2] = {1840.63, 1980.00};
const double zeroBurstDurationInterval[2] = {538.0, 680.0};
// ===================== END Receiver Variables =====================
// ===================== START HOST COMMUNICATION VARIABLES =====================
const unsigned int maxMsgSize = 255;      // 255 chunks maximum per msg
const int maxByteNumber = maxMsgSize * 3; // maxChunkNumber == 255 * 3 content bytes each chunk = 765 Bytes max to fit into the array
const int HOST_AUTHORIZE_MODULE_VALUE = 187;
const char HOST_INCOMING_PAYLOAD_COMMAND = 'S';
const char HOST_DATA_RECEPTION_ERROR = 'E';
const char HOST_PAYLOAD_RECEPTION_COMMAND = 'D';
const char HOST_TRANSMISSION_SUCCESS_COMMAND = 'X';
const char HOST_RESET_MODULE_COMMAND = 'R';
const char HOST_HARD_RESET_MODULE_COMMAND = 'H';
unsigned int expectedBytesNumber = 0;
byte bytes[maxByteNumber];
boolean dataReceivedFromHost = false;
boolean dataPrepared = false;
boolean authorizationInitStarted = false;
boolean awaithostResponse = false;
boolean autoTransmitGranted = false;
boolean blockHardReset = false;
// ===================== END HOST COMMUNICATION VARIABLES =====================
const unsigned int msgReferenceSequence = 0x80000000;
const unsigned int msgBitNumber = 32;
const unsigned int cmdReferenceSequence = 0x80;
const unsigned int cmdBitNumber = 8;
const unsigned int startChunkReferenceSequence = 0x8000;
const unsigned int startChunkBitNumber = 16;
unsigned int codeChunkNumber = 0;
unsigned int contentBuffer[maxMsgSize] = {0};

const unsigned int timeoutThreshold = startBurstDurationInterval[1] + startPauseDurationInterval[1] + msgBitNumber * (oneBurstDurationInterval[1] + (2 * zeroBurstDurationInterval[1])) + endBurstDurationInterval[1];
const int maxTimeouts = 50;
const unsigned int successIndicatorThreshold = 10;

unsigned long timeoutTimer = 0;
unsigned long successIndicatorTimer = 0;
int timeoutCounter = 0;
int incomingBit;
boolean isTransmissionInitialized = false;
boolean chunkReceptionSuccessful = false;
boolean toggleIndicator = false;


//TESTING
// ===================== Measurement Variables Start =====================
unsigned long totalTransmissionTimer = 0;    // millis
unsigned long totalTransmissionDuration = 0; // millis
unsigned int msgCounter = 0;
unsigned int reqResCounter = 0;

unsigned int nextMsgCounter = 0;
unsigned int repeatCounter = 0;
unsigned int validMessageCounter = 0;
unsigned int chunkNumberCounter = 0;
unsigned int stopCounter = 0;
unsigned int startCounter = 0;
unsigned int cmdNotFoundCounter = 0;
unsigned int unknownCommandCounter = 0;
unsigned int measurementIndex = 0;
unsigned int timeoutCounterTest = 0;
// ===================== Measurement Variables End =====================

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    delay(1000);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(FEEDBACK_LED, OUTPUT);
    pinMode(RECV_PIN, INPUT);

    pinMode(BTN_PIN, INPUT);

    pinMode(IR_LED_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(IR_LED_PIN, LOW);
    digitalWrite(FEEDBACK_LED, LOW);
    initDataPrep(msgReferenceSequence);

    initTransmitter(frequency, wavePeriodDuration,
                    leadPulseLength, leadPauseLength, stopPulseLength,
                    BitTime, HIGHBitTime,
                    msgBitNumber, msgReferenceSequence,
                    cmdBitNumber, cmdReferenceSequence);
    initReceiver(calibrate,
                 startBurstDurationInterval, startPauseDurationInterval, endBurstDurationInterval,
                 oneBurstDurationInterval, zeroBurstDurationInterval,
                 timeoutThreshold);
}

void indicateSystemState(int repeatNum, int delayNum)
{
    for (int i = 0; i < repeatNum; i++)
    {
        digitalWrite(FEEDBACK_LED, HIGH);
        delay(delayNum);
        digitalWrite(FEEDBACK_LED, LOW);
        if (repeatNum != 1)
            delay(delayNum);
    }
}

// Updates the superState of the Module, given the corresponding value "state"
void updateSuperStateMain(int state)
{
    if (superState == NONE && state == RECEIVER)
    {
        superState = RECEIVER;
        updateSuperStateRecv(RECEIVER);
    }
    else if (superState == NONE && state == SENDER)
    {
        totalTransmissionTimer = millis();
        superState = SENDER;
        updateSuperStateRecv(SENDER);
    }
    else if (state == NONE)
    {
        superState = NONE;
        updateSuperStateRecv(NONE);
    }
}

void clearDataPlaceholders()
{
    for (unsigned int i = 0; i < expectedBytesNumber; i++)
        bytes[i] = 0;
    expectedBytesNumber = 0;
    for (unsigned int i = 0; i < codeChunkNumber; i++)
        contentBuffer[i] = 0;
    codeChunkNumber = 0;
    timeoutCounter = 0;
    timeoutTimer = 0;
    successIndicatorTimer = 0;
    resetTransmitter();
    resetReceiver();
}

// Completely resets the module as well as host connection.
void resetAndDropHostConnection()
{
    indicateSystemState(1, 1200);
    updateSuperStateMain(NONE);
    digitalWrite(FEEDBACK_LED, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    // Set all variables to default
    authorizationInitStarted = false;
    moduleIsConnected = false;
    dataReceivedFromHost = false;
    dataPrepared = false;
    autoTransmitGranted = false;
    transmitPermission = WAITING_FOR_PERMISSION;
    clearDataPlaceholders();
}
// Reset module.
void resetModule()
{
    indicateSystemState(2, 100);
    updateSuperStateMain(NONE);
    // Set all variables to default except host connection variables, so
    // data can still be received by the host.
    autoTransmitGranted = false;
    dataReceivedFromHost = false;
    dataPrepared = false;
    transmitPermission = WAITING_FOR_PERMISSION;
    clearDataPlaceholders();
}
// Reset module with auto respond permission.
void prepareAutoResponse()
{
    indicateSystemState(1, 250);
    updateSuperStateMain(NONE);
    // Set all variables to default except host connection variables, so
    // data can still be received by the host.
    // Additionally enable automatic transmission of requested data.
    autoTransmitGranted = true;
    dataReceivedFromHost = false;
    dataPrepared = false;
    transmitPermission = WAITING_FOR_PERMISSION;
    clearDataPlaceholders();
}

//TESTING
void updateTransmissionTimer()
{
    totalTransmissionTimer = millis();
}

//TESTING
void loggerMain(unsigned int data)
{
    // if(calibrate || testing){
    // REMOVE in final version
    // Serial.println();
    // REMOVE in final version
    // Serial.print("; RESPONSE: ");
    // // REMOVE in final version
    // if (data == NEXT_PKT)
    // {
    //     // REMOVE in final version
    //     Serial.println("NEXT MESSAGE");
    // }
    // else if (data == REPEAT)
    // {
    //     // REMOVE in final version
    //     Serial.println("REPEAT");
    // }
    // else if (data == VALID_MESSAGE)
    // {
    //     // REMOVE in final version
    //     Serial.println("CONTENT");
    // }
    // else if (data == CHUNK_NUMBER)
    // {
    //     // REMOVE in final version
    //     Serial.println("CHUNK NUMBER");
    // }
    // else if (data == STOP)
    // {
    //     // REMOVE in final version
    //     Serial.println("STOP");
    // }
    // else if (data == START)
    // {
    //     // REMOVE in final version
    //     Serial.println("START");
    // }
    // else if (data == CMD_NOT_FOUND_ERROR)
    // {
    //     // REMOVE in final version
    //     Serial.println("COMMAND NOT FOUND");
    // }
    // else
    // {
    //     // REMOVE in final version
    //     Serial.println(data);
    // }
    // }
    if (data == NEXT_PKT)
    {
        nextMsgCounter++;
    }
    else if (data == REPEAT)
    {
        repeatCounter++;
    }
    else if (data == VALID_MESSAGE)
    {
        validMessageCounter++;
    }
    else if (data == CHUNK_NUMBER)
    {
        chunkNumberCounter++;
    }
    else if (data == STOP)
    {
        stopCounter++;
    }
    else if (data == START)
    {
        startCounter++;
    }
    else if (data == 187)
    {
        timeoutCounterTest++;
    }
    else if (data == CMD_NOT_FOUND_ERROR)
    {
        cmdNotFoundCounter++;
    }
    else
    {
        unknownCommandCounter++;
    }
}
//TESTING
void updateMeasurementMain()
{
    totalTransmissionDuration = millis() - totalTransmissionTimer;
    double duration = totalTransmissionDuration / 1000.0;
    measurementIndex++;
    Serial.println();
    Serial.print("INDEX: ");
    Serial.println(measurementIndex);
    Serial.print("DURATION: ");
    Serial.println(duration);
    Serial.print("REQ/RES: ");
    Serial.println(reqResCounter);
    Serial.print("INTERACTION: ");
    Serial.println(reqResCounter + msgCounter);
    Serial.print("NEXT_PKT: ");
    Serial.println(nextMsgCounter);
    Serial.print("REPEAT: ");
    Serial.println(repeatCounter);
    Serial.print("PACKET: ");
    Serial.println(msgCounter);
    Serial.print("CHUNK_NUMBER: ");
    Serial.println(chunkNumberCounter);
    Serial.print("STOP: ");
    Serial.println(stopCounter);
    Serial.print("START: ");
    Serial.println(startCounter);
    Serial.print("TIMEOUT: ");
    Serial.println(timeoutCounterTest);
    Serial.print("CMD_NOT_FOUND: ");
    Serial.println(cmdNotFoundCounter);
    Serial.print("UNKNOWN_CMD: ");
    Serial.println(unknownCommandCounter);
    resetMeasurement();
}

void resetMeasurement(){
    totalTransmissionTimer = 0;    // millis
    totalTransmissionDuration = 0; // millis
    msgCounter = 0;
    reqResCounter = 0;
    nextMsgCounter = 0;
    repeatCounter = 0;
    validMessageCounter = 0;
    chunkNumberCounter = 0;
    stopCounter = 0;
    startCounter = 0;
    cmdNotFoundCounter = 0;
    unknownCommandCounter = 0;
    timeoutCounterTest = 0;
}

// Sends response message passed as "responseMessage"
void sendResponse(unsigned int responseMessage)
{
    // REMOVE IN FINAL VERSION
    loggerMain(responseMessage);
    timeoutCounter = 0;
    if (responseMessage == CHUNK_NUMBER)
    {
        //TESTING
        reqResCounter++;
        // retransmitting chunk number in case of REPEAT command
        sendIRCode(codeChunkNumber, startChunkBitNumber, startChunkReferenceSequence);
        return;
    }
    //TESTING
    reqResCounter++;
    sendIRCode(responseMessage, cmdBitNumber, cmdReferenceSequence);
}

// Tells transmitter to either repeat or send next message chunk
void updateCodeChunkIndexMain(unsigned int n)
{
    timeoutCounter = 0;
    //TESTING
    msgCounter++;
    updateCodeChunkIndexTransm(n);
}

void updateResponseTimer()
{
    timeoutTimer = micros();
}

// This function resets SENDER module and RECEIVER module, if no response
// or Signal is received or sent during the period of maxTimeouts times timeoutThreshold
void checkResponseTimeout()
{
    if (superState != NONE && (micros() > (timeoutTimer + timeoutThreshold)))
    {
        updateResponseTimer();
        // Resets Module
        if (superState != NONE && isTransmissionInitialized == true && timeoutCounter >= maxTimeouts)
            resetModule();
        // Sends repeat signal after response timeout
        else if (superState == SENDER)
        {
            //TESTING
            reqResCounter++;
            timeoutCounterTest++;
            sendIRCode(REPEAT, cmdBitNumber, cmdReferenceSequence);
        }
        timeoutCounter++;
    }
    else if (isTransmissionInitialized == false && timeoutCounter >= maxTimeouts * 3)
        resetModule();
}

// This is needed for timeout reset, as it can only reset, if initialization has taken place.
// It can only reset after initialization of a connection because finding the connection could take some time.
void setTransmissionInitializedMain(boolean isInit)
{
    if(!isTransmissionInitialized){
        resetMeasurement();
    }
    isTransmissionInitialized = true;
}

void updateSuccessIndicator()
{
    chunkReceptionSuccessful = true;
    successIndicatorTimer = millis();
}

// Lets an LED blink when data has been received.
// This serves as feedback for the user
void successIndicator()
{
    if (chunkReceptionSuccessful && !toggleIndicator)
    {
        chunkReceptionSuccessful = false;
        toggleIndicator = true;
        digitalWrite(FEEDBACK_LED, HIGH);
    }
    else if (!chunkReceptionSuccessful && toggleIndicator && (millis() > (successIndicatorTimer + successIndicatorThreshold)))
    {
        toggleIndicator = false;
        digitalWrite(FEEDBACK_LED, LOW);
    }
}

void buildHostConnection()
{
    int registrationByte = 0;
    if (Serial.available() > 0 && !moduleIsConnected)
    {
        registrationByte = Serial.read();
        if (!authorizationInitStarted)
        {
            registrationByte = registrationByte ^ HOST_AUTHORIZE_MODULE_VALUE;
            Serial.println(registrationByte);
            // Add EOF character at the end of the String.
            // This is required to successfully receive strings of all lengths by the 
            // host device.
            Serial.println((char)26); 
            Serial.read();
            authorizationInitStarted = true;
            digitalWrite(LED_BUILTIN, LOW);
        }
        else if (registrationByte == HOST_AUTHORIZE_MODULE_VALUE)
            moduleIsConnected = true;
        else
            authorizationInitStarted = false;
    }
}

// This function is very critical to timing! No printlining allowed in this function besides
// the existing commands.
void listenForIncomingHostData()
{
    // Each delay in this function keeps the MC from resetting.
    if (Serial.available() > 0 && moduleIsConnected && !dataReceivedFromHost)
    {
        int incomingBytesNumber = 0;

        if (Serial.peek() == HOST_INCOMING_PAYLOAD_COMMAND)
        {
            // This manually clears the serial input 
            Serial.read();
            incomingBytesNumber = Serial.parseInt();
            Serial.read();
            delay(1);
            // Checks if byte number is within bounds
            if (incomingBytesNumber <= maxByteNumber && incomingBytesNumber != 0)
            {
                blockHardReset = true;
                expectedBytesNumber = incomingBytesNumber;
                Serial.println(HOST_INCOMING_PAYLOAD_COMMAND);
                Serial.readBytesUntil('\n', bytes, incomingBytesNumber);
                Serial.println(HOST_PAYLOAD_RECEPTION_COMMAND);
                dataReceivedFromHost = true;
                blockHardReset = false;                
            }
            else
            {
                Serial.println(HOST_DATA_RECEPTION_ERROR);
                resetModule();
            }
        }
    }
}

void listenForHostCommands()
{
    if (Serial.available() > 0 && !blockHardReset)
    {
        if (Serial.peek() == HOST_HARD_RESET_MODULE_COMMAND)
        {
            Serial.read();
            resetAndDropHostConnection();
            return;
        }
        else if (Serial.peek() == HOST_RESET_MODULE_COMMAND)
        {
            Serial.read();
            resetModule();
            return;
        }
    }
}

void prepareDataForTransmission()
{
    if (moduleIsConnected && dataReceivedFromHost && !dataPrepared)
    {
        dataPrepared = true;
        prepareData(expectedBytesNumber, bytes);
    }
}

// ENDPOINT: pass data to computer from here
void moduleResponseToHost()
{
    if (superState == SENDER)
        resetModule();
    else if (superState == RECEIVER)
    {
        Serial.println(HOST_TRANSMISSION_SUCCESS_COMMAND);
        for (unsigned int i = 0; i < expectedBytesNumber; i++)
            Serial.print((char)bytes[i]);
        // Add EOF character at the end of the String.
        // This is required to successfully receive strings of all lengths by the 
        // host device.
        Serial.println((char)26); 
        prepareAutoResponse();
    }
}

void fillByteContentInMain(unsigned int byteNumber, unsigned int *content)
{
    expectedBytesNumber = byteNumber;
    for (unsigned int i = 0; i < byteNumber; i++)
        bytes[i] = content[i];
}

// This function is called in dataPrep and fills the contentBuffer as well as codeChunkNumber
// It either fills the contentBuffer, if data must be transmitted over to another module,
// or it fills the contentBuffer with incoming data, which should be transmitted to another device.
void fillBufferInMain(unsigned int chunkNumber, unsigned int *content, boolean dataRcvd)
{
    codeChunkNumber = chunkNumber;
    for (unsigned int i = 0; i < codeChunkNumber; i++)
        contentBuffer[i] = content[i];
    if (dataRcvd)
        decodeData(codeChunkNumber, contentBuffer);
}

boolean inspectChunkContent(int *sequence)
{
    return correctSequence(sequence);
}

boolean toggleTest = true;
int testTimer = millis();
void loop()
{
    if (!testing)
    {
        /** REGULAR CODE*/

        //============== Load Data from Serial START ==============
        listenForHostCommands();
        if (transmitPermission == WAITING_FOR_PERMISSION)
        {
            buildHostConnection();
            listenForIncomingHostData();
            prepareDataForTransmission();
        }
        //============== Load Data from Serial END ==============
        //============== Transmission Code START ==============
        if (moduleIsConnected && dataReceivedFromHost && dataPrepared && transmitPermission == WAITING_FOR_PERMISSION)
        {
            transmitPermission = PERMISSION_GRANTED;
            digitalWrite(FEEDBACK_LED, HIGH);
        }
        checkResponseTimeout();
        successIndicator();

        int buttonState = digitalRead(BTN_PIN);
        if ((!buttonState ^ autoTransmitGranted) && transmitPermission == PERMISSION_GRANTED)
        {
            transmitPermission = PERMISSION_EXPIRED;
            autoTransmitGranted = false;
            digitalWrite(FEEDBACK_LED, LOW);
            updateSuperStateMain(SENDER);
            loadIRCodeSequence(codeChunkNumber, contentBuffer);
            //TESTING
            reqResCounter++;
            sendIRCode(codeChunkNumber, startChunkBitNumber, startChunkReferenceSequence);
            updateResponseTimer();
        }
        incomingBit = digitalRead(RECV_PIN);
        receiveIRCode(incomingBit);
        //============== Transmission Code END ==============
    }
    //TESTING
    else
    {

        /** SENDER MODULE TEST CODE*/
        checkResponseTimeout();
        successIndicator();
        int buttonState = digitalRead(BTN_PIN);
        // if (buttonState == 0 && transmitPermission == PERMISSION_GRANTED)
        if (buttonState == 0 && toggleTest)
        {
            // remove in final version
            testTimer = millis();
            toggleTest = false;
            int testByteNumber = 3;
            byte msgHello[] = {'h','e','l','l','o',' ','w','o','r','l','d',',',' ','i','t','s',' ','m','e',' ','m','a','r','i','o','!','!'};
            // byte msg[] = {'<','K','a','b','e','l','B','o','x','-','A','1','B','2','>','<','d','r','e','i','h','u','n','d','e','r','t','d','r','e','i','u','n','d','d','r','e','i','s','s','i','g','3','>'};
            byte wifiReq[] = {'w', 'i', 'f'};
            byte actReq[] = {'a', 'c', 't'};
            byte updateDBReq[] = {'u', 'D', 'B'};
            prepareData(testByteNumber, msgHello);

            transmitPermission = PERMISSION_EXPIRED;
            digitalWrite(FEEDBACK_LED, LOW);
            updateSuperStateMain(SENDER);
            updateSuperStateRecv(SENDER);
            loadIRCodeSequence(codeChunkNumber, contentBuffer);
            //remove in final version
            reqResCounter++;
            if (!calibrate)
            {
                sendIRCode(codeChunkNumber, startChunkBitNumber, startChunkReferenceSequence);
            }
            updateResponseTimer();
            // toggleTest = true;
        }
        incomingBit = digitalRead(RECV_PIN);
        receiveIRCode(incomingBit);
        if (toggleTest == false && millis() > testTimer+500){
            testTimer = millis();
            toggleTest = true;
        }
        //============== Transmission Code END ==============
    }
}
