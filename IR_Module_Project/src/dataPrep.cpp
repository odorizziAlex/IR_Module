#include <Arduino.h>
#include <dataPrep.h>

// Source for this code: https://www.youtube.com/watch?v=b3NxrZOu_CE&t=167s

const unsigned int maxMsgSize = 255;
const unsigned int maxByteNumberTotal = 765; // maxChunkNumber == 255 * 3 content bytes each chunk = 765
const unsigned int totalContentBitNumber = 32;
const unsigned int actualContentBitNumber = 24;
const unsigned int byteBitNumber = 8;
const unsigned int totalChunkSizeInByte = 4;
const unsigned int maxContentByteNumberPerChunk = 3;

// ===================== START Create Parity Variables =====================

const unsigned int parityIndexNumber = 15;
const unsigned int contentIndexesInHammingSequence[] = {6, 7, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
const unsigned int parityBitIndexes[] = {0, 1, 2, 4, 8, 16};
const unsigned int chunkIndexBits[] = {3,5};
const unsigned int P1Indexes[] = {9, 17, 25, 3, 11, 19, 27, 5, 13, 21, 29, 7, 15, 23, 31};
const unsigned int P2Indexes[] = {10, 18, 26, 3, 11, 19, 27, 6, 14, 22, 30, 7, 15, 23, 31};
const unsigned int P3Indexes[] = {12, 20, 28, 5, 13, 21, 29, 6, 14, 22, 30, 7, 15, 23, 31};
const unsigned int P4Indexes[] = {9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31, 24};
const unsigned int P5Indexes[] = {17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

unsigned int P1 = 0;
unsigned int P2 = 0;
unsigned int P3 = 0;
unsigned int P4 = 0;
unsigned int P5 = 0;
unsigned int P6 = 0;

// ===================== END Create Parity Variables =====================

unsigned int finalContentBytes[maxByteNumberTotal];
unsigned int finalContent[maxMsgSize];
unsigned int finalContentChunk[totalContentBitNumber];

unsigned int msgIndex = 0;
unsigned int contentIndex = 0;
unsigned int byteIndex = 0;

unsigned int totalByteNumber;
unsigned int neededChunkNumber;

unsigned int byteReferenceSequence;
unsigned int contentReferenceSequence;

void initDataPrep(unsigned int msgReferenceSeq)
{
    contentReferenceSequence = msgReferenceSeq;
}

void resetAllValues()
{
    P1 = 0;
    P2 = 0;
    P3 = 0;
    P4 = 0;
    P5 = 0;
    P6 = 0;

    for (unsigned int i = 0; i < maxMsgSize; i++)
        finalContent[i] = 0;
    for (unsigned int i = 0; i < totalContentBitNumber; i++)
        finalContentChunk[i] = 0;
    totalByteNumber = 0;
    neededChunkNumber = 0;
    contentIndex = 0;
    byteIndex = 0;
}

void resetChunkValues()
{
    for (unsigned int i = 0; i < totalContentBitNumber; i++)
        finalContentChunk[i] = 0;
    contentIndex = 0;
}

void convertDecimalTo8BitBinary(unsigned int dec, unsigned int *contentBits)
{
    unsigned int n = dec;
    unsigned int seq[byteBitNumber] = {};
    unsigned int index = 0;
    // Sorts LSB first
    while (n > 0)
    {
        seq[index] = n % 2;
        n = n / 2;
        index++;
    }
    // Save invert in final byte array
    for (int i = byteBitNumber - 1; i >= 0; i--)
    {
        contentBits[contentIndex] = seq[i];
        contentIndex++;
    }
}

unsigned int decodeChunkToInteger(unsigned int *seq, int bitNumber)
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

void calculateParityBits()
{

    P1 = 0;
    P2 = 0;
    P3 = 0;
    P4 = 0;
    P5 = 0;

    // Counting the ones on specific positions for each parity bit
    unsigned int position;
    for (unsigned int i = 0; i < parityIndexNumber; i++)
    {
        position = P1Indexes[i];
        if (finalContentChunk[position] == 1)
            P1++;
        position = P2Indexes[i];
        if (finalContentChunk[position] == 1)
            P2++;
        position = P3Indexes[i];
        if (finalContentChunk[position] == 1)
            P3++;
        position = P4Indexes[i];
        if (finalContentChunk[position] == 1)
            P4++;
        position = P5Indexes[i];
        if (finalContentChunk[position] == 1)
            P5++;
    }
    // If the number of ones for a parity bit is odd, a 1 will be added at the corresponding position in the finalContentChunk Array
    if (P1 % 2 == 1)
        finalContentChunk[parityBitIndexes[1]] = 1;
    if (P2 % 2 == 1)
        finalContentChunk[parityBitIndexes[2]] = 1;
    if (P3 % 2 == 1)
        finalContentChunk[parityBitIndexes[3]] = 1;
    if (P4 % 2 == 1)
        finalContentChunk[parityBitIndexes[4]] = 1;
    if (P5 % 2 == 1)
        finalContentChunk[parityBitIndexes[5]] = 1;
}

void addIndexBits(unsigned int index){
    for (int i = 0; i < 2; i++)
    {
        if(index & 0b10)
            finalContentChunk[chunkIndexBits[i]] = 1;
        else
            finalContentChunk[chunkIndexBits[i]] = 0;
        index<<=1;
    }
}

void addExtendedHammingBit()
{
    P6 = 0;
    // 1, if the number of all 1s in the finalContentChunk array is odd
    for (unsigned int i = 0; i < totalContentBitNumber; i++)
        if (finalContentChunk[i] == 1)
            P6++;
    if (P6 % 2 == 1)
        finalContentChunk[0] = 1;
}

// Builds each 32 Bit chunk with parity bits and content values.
void fillContentToChunk(unsigned int *data, unsigned int index)
{
    contentIndex = 0;
    // This is a placeholder list for all content bits in a chunk.
    unsigned int contentBits[actualContentBitNumber] = {};

    // This decodes three bytes into binary and saves it to the contentBits array.
    for (unsigned int i = 0; i < maxContentByteNumberPerChunk; i++)
    {
        convertDecimalTo8BitBinary(data[byteIndex], contentBits);
        byteIndex++;
    }

    // Fills all content bits at the right position for hamming correction in the final chunk.
    for (unsigned int i = 0; i < actualContentBitNumber; i++)
    {
        unsigned int position = contentIndexesInHammingSequence[i];
        finalContentChunk[position] = contentBits[i];
    }

    // Calculates parity bits and safes them at the right position in finalContentChunk list.
    addIndexBits(index);
    calculateParityBits();
    addExtendedHammingBit();
}

void saveChunkToFinalSequence(unsigned int chunkIndex)
{    
    unsigned int sequenceValueInt = decodeChunkToInteger(finalContentChunk, totalContentBitNumber);
    finalContent[chunkIndex] = sequenceValueInt;
}

unsigned int findBitflipPosition(int *hammingSeq)
{
    int onePositions[totalContentBitNumber];
    unsigned int index = 0;

    for (unsigned int i = 0; i < totalContentBitNumber; i++)
    {
        if (hammingSeq[i] == 1)
        {
            onePositions[index] = i;
            index++;
        }
    }

    int errorPosition = 0;

    for (unsigned int i = 0; i < index; i++)
        errorPosition = errorPosition ^ onePositions[i];

    return errorPosition;
}

void decodeChunkToBitArray(unsigned int value, unsigned int *resultSeq)
{
    for (unsigned int i = 0; i < totalContentBitNumber; i++)
    {
        if (value & contentReferenceSequence)
            resultSeq[i] = 1;
        value <<= 1;
    }
}

boolean checkP6(int *seq)
{
    unsigned int numberOfOnes = 0;
    for (unsigned int i = 0; i < totalContentBitNumber; i++)
    {
        if (seq[i] == 1 && i != 0)
            numberOfOnes++;
    }
    unsigned int onesNumberOdd = numberOfOnes % 2;

    if (onesNumberOdd == seq[0])
        return true;
    return false;
}

void performErrorCorrection(unsigned int ePos, int *seq)
{
    if ((ePos == 0 && !checkP6(seq)) || (ePos != 0 && !checkP6(seq)))
    {
        if (seq[ePos] == 0)
            seq[ePos] = 1;
        else
            seq[ePos] = 0;
    }
}

void decodeChunk(unsigned int data)
{
    unsigned int hammingSequence[totalContentBitNumber] = {};

    decodeChunkToBitArray(data, hammingSequence);
    
    for (unsigned int i = 0; i < totalContentBitNumber; i++)
        finalContentChunk[i] = hammingSequence[i];
}

void extractContentBytes()
{
    unsigned int contentBits[actualContentBitNumber] = {};
    // Extract all content Bits from chunk
    for (unsigned int i = 0; i < actualContentBitNumber; i++)
    {
        unsigned int position = contentIndexesInHammingSequence[i];
        contentBits[i] = finalContentChunk[position];
    }

    unsigned int index = 0;
    // Split content bytes into integer values and save each value as a byte
    for (unsigned int i = 0; i < maxContentByteNumberPerChunk; i++)
    {
        unsigned int tempByteBits[byteBitNumber] = {};
        unsigned int tempByte = 0;
        // Split content bits into byte chunks for decoding
        for (unsigned int j = 0; j < byteBitNumber; j++)
        {
            tempByteBits[j] = contentBits[index];
            index++;
        }
        tempByte = decodeChunkToInteger(tempByteBits, byteBitNumber);
        finalContentBytes[byteIndex] = tempByte;
        byteIndex++;
    }
}

// Decodes incoming data.
void decodeData(unsigned int contentChunkNumber, unsigned int *data)
{
    resetAllValues();
    unsigned int incomingRawData[maxMsgSize] = {};

    neededChunkNumber = contentChunkNumber;
    totalByteNumber = neededChunkNumber * 3;

    for (unsigned int i = 0; i < contentChunkNumber; i++)
        incomingRawData[i] = data[i];

    for (unsigned int i = 0; i < neededChunkNumber; i++)
    {
        decodeChunk(incomingRawData[i]);
        extractContentBytes();
        resetChunkValues();
    }

    fillByteContentInMain(totalByteNumber, finalContentBytes);
}

// Prepares data for transmission.
void prepareData(unsigned int byteNum, byte *data)
{
    resetAllValues();
    unsigned int rawData[maxMsgSize] = {};

    // Set passed value as instance Variable.
    totalByteNumber = byteNum;

    // Make byteNumber dividable by 3.
    if (totalByteNumber % maxContentByteNumberPerChunk != 0)
        totalByteNumber += (maxContentByteNumberPerChunk - (totalByteNumber % maxContentByteNumberPerChunk));

    // Set new chunk number.
    neededChunkNumber = totalByteNumber / maxContentByteNumberPerChunk;

    for (unsigned int i = 0; i < byteNum; i++)
        rawData[i] = data[i];

    // Iterate over all raw Values, and convert the whole sequence into:
    // 1. 32 Bit Chunk with 3 content bytes, 6 parity bits and two index bits in the suiting sequence.
    // 2. Save this value as an Integer for transmission later.
    for (unsigned int i = 0; i < neededChunkNumber; i++)
    {
        fillContentToChunk(rawData, i);
        saveChunkToFinalSequence(i);
        resetChunkValues();
    }

    fillBufferInMain(neededChunkNumber, finalContent, false);
}

boolean correctSequence(int * data){
    int errorPos;
    errorPos = findBitflipPosition(data);

    // This condition is true, when data contains more than one bitflip.
    // Even though faulty sequences get already discarded in receiver.cpp
    // sequences, where bits are flipped from 0 to one or vice versa have to
    // be identified.
    if (errorPos != 0 && checkP6(data))
        return false;

    // This condition is true, when there is no bitflip in this sequence.
    // When errorPos quals 0 and checkP6 equals true, the sequence does not contain
    // bitflips.
    else if(errorPos == 0 && checkP6(data))
        return true;
    performErrorCorrection(errorPos, data);
    return true;
}