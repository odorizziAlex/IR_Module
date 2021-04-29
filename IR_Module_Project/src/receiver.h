// RECEIVER METHODS
void initReceiver(boolean calibrate, const double* startBrstDInterval,
    const double* startPsDInterval,const double* endBrstDInterval, 
    const double* oneBrstDInterval, const double* zeroBrstDInterval, 
    const unsigned int timeoutThreashold);
    // const unsigned int msgBitNum, const unsigned int referenceBitSeq);

void receiveIRCode(int incomingBit);

void sendResponse(unsigned int MSG);

void updateSuperStateMain(int state);

void updateSuperStateRecv(int state);

void updateCodeChunkIndexMain(unsigned int n);

void setTransmissionInitializedMain(boolean isInit);

void resetReceiver();

void updateSuccessIndicator();

void fillBufferInMain(unsigned int chunkNumber, unsigned int * content, boolean dataRcvd);

boolean inspectChunkContent(int *sequence);

void moduleResponseToHost();

//TESTING
void loggerMain(unsigned int code);

//TESTING
void updateTransmissionTimer();

//TESTING
void updateMeasurementMain();
void resetMeasurement();