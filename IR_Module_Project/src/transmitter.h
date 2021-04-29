void initTransmitter(unsigned int freq,unsigned int wavePrdD,
    unsigned int lPulseLen,unsigned int lPauseLen,unsigned int sPulseLen,
    unsigned int lowBTime,unsigned int highBTime, 
    const unsigned int msgBitNum, const unsigned int msgReferenceBitSeq,
    const unsigned int cmdBitNum, const unsigned int cmdReferenceBitSeq);

void loadIRCodeSequence(unsigned int chunkNum, unsigned int * code);

void sendIRCode(unsigned int code, unsigned int bitNumber, unsigned int referenceCode);

void updateCodeChunkIndexTransm(unsigned int n);

void updateResponseTimer();

void resetTransmitter();

void moduleResponseToHost();