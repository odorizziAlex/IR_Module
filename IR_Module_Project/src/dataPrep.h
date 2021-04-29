void initDataPrep(unsigned int msgReferenceSeq);

void prepareData(unsigned int byteNum, byte * data);

void decodeData(unsigned int contentChunkNumber, unsigned int *data);

void fillBufferInMain(unsigned int chunkNumber, unsigned int * content, boolean dataRcvd);

void fillByteContentInMain(unsigned int byteNumber, unsigned int * content);

void decodeChunkToBitArray(unsigned int value, unsigned int *resSeq);

boolean correctSequence(int * data);