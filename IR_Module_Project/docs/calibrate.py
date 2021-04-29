import serial
import time
from pandas import DataFrame

totalNumberSignals = 100

def updateProgressBar(iteration,maximumIterations):
    printProgressBar(iteration, maximumIterations, prefix = 'Progress:', suffix = 'Complete', length = 50)

def printProgressBar (iteration, total, prefix = 'Progress', suffix = 'Complete', decimals = 1, length = 500, fill = '█', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix), end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()

def analyze(data):
    # referenceSequence = "0b11111111111111110000000000000000"
    startSignals = []
    startPause = []
    endSignals = []
    ones = []
    zeros = []

    for line in data:
        if(len(line) != 34):
            continue;
        startSignals.append(int(line[0]))
        startPause.append(int(line[1]))
        endSignals.append(int(line[len(line)-1]))
        for index, item in enumerate(line):
            if(index == 0 or index == len(line)-1 or index == 1):
                continue;
            elif(index <= 16):
                ones.append(int(item))
            else:
                zeros.append(int(item))
  
    print()
    dfStart = DataFrame(startSignals, columns=["startSignals"])
    qs = dfStart.startSignals.quantile([0,0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99,1])
    print(qs)
    dfPause = DataFrame(startPause, columns=["startPause"])
    qsp = dfPause.startPause.quantile([0,0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99,1])
    print(qsp)
    dfEnd = DataFrame(endSignals, columns=["endSignals"])
    qe = dfEnd.endSignals.quantile([0,0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99,1])
    print(qe)
    dfOnes = DataFrame(ones, columns=["oneSignals"])
    qo = dfOnes.oneSignals.quantile([0,0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99,1])
    print(qo)
    dfZeros = DataFrame(zeros, columns=["zeroSignals"])
    qz = dfZeros.zeroSignals.quantile([0,0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99,1])
    print(qz)



def readValues():
    time.sleep(1)
    print("reading...");
    countdown = 0
    data = []
    
    updateProgressBar(countdown, totalNumberSignals)
    with serial.Serial("/dev/tty.usbserial-01486A59", 115200, timeout=1) as ser:
        while(countdown < totalNumberSignals):
            npt = str(ser.readline()).split("'")[1]
            if(npt != ""):
                countdown = countdown+1
                npt = npt.split(";")
                npt.pop(len(npt)-1)
                # print(npt)
                data.append(npt)
                updateProgressBar(countdown, totalNumberSignals)
        ser.close()
    analyze(data)


readValues()