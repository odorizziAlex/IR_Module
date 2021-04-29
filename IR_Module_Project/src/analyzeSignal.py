import serial
import time
import csv

totalNumberSignals = 100

def updateProgressBar(iteration,maximumIterations, signal):
    printProgressBar(iteration + 1, maximumIterations, signal, prefix = 'Progress:', suffix = 'Complete', length = 50)

def printProgressBar (iteration, total, extra, prefix = 'Progress', suffix = 'Complete', decimals = 1, length = 500, fill = '█', printEnd = "\r"):
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
    print('\r%s |%s| %s%% %s \n %s' % (prefix, bar, percent, suffix, extra), end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()

def createCSVFile(data, fileName, testDistance, bitNumber):
    try:
        with open(fileName, 'w') as file:
            if(bitNumber == "32"):
                writer = csv.DictWriter(file, fieldnames=["distance(cm)","start","1","2","3","4","5",	
                "6","7","8","9","10","11","12","13","14","15","16","17","18","19","20",
                "21","22","23","24","25","26","27","28","29","30","31","32","end","index","code"], delimiter=";")
            elif(bitNumber == "16"):
                writer = csv.DictWriter(file, fieldnames=["distance(cm)","start","1","2","3","4","5",	
                "6","7","8","9","10","11","12","13","14","15","16","end","index","code"], delimiter=";")
            elif(bitNumber == "64"):
                writer = csv.DictWriter(file, fieldnames=["distance(cm)","start","1","2","3","4","5",	
                "6","7","8","9","10","11","12","13","14","15","16","17","18","19","20",
                "21","22","23","24","25","26","27","28","29","30","31","32","33","34","35","36","37",	
                "38","39","40","41","42","43","44","45","46","47","48","49","50","51","52",
                "53","54","55","56","57","58","59","60","61","62","63","64","end","index","code"], delimiter=";")
            writer.writeheader()
            for row in data:
                line = str(testDistance)+";"
                for item in row:
                    line = line + str(item) + ";"
                line = line+"\n"
                file.write(line)
    except :
        print("filepath error ")
        print(data)
    finally:
        file.close()

def preprocessData(data):
    data = str(data[0])
    data = data.split(";")
    data[0]= data[0].replace("b'","")
    data[len(data)-1] = data[len(data)-1].replace("\\r\\n'","")
    return data

def decode():
    time.sleep(2)
    countDown = 0

    bitNumber = input("How many bits does the Signal have? (16, 32 or 64): ")
    distance = input("Type testing distance (in cm): ")
    outputFileName = input("1. RESET RECEIVER!\n2. Type filename of output file (no suffix needed): ")
    print ("listening...")
    updateProgressBar(countDown, totalNumberSignals, "None")
    data = []
    with serial.Serial("/dev/tty.usbserial-01486A59", 115200, timeout=1) as ser:
        while(True):
            npt = ser.readlines()
            if(npt != []):
                npt = preprocessData(npt)
                data.append(npt)
                updateProgressBar(countDown, totalNumberSignals, str(npt[len(npt)-2])+" # "+str(npt[len(npt)-1]))
                countDown = countDown + 1
            if(len(data) == totalNumberSignals):
                break;
    # createCSVFile(data, "../../SignalDistanceTests/HandheldIRLEDStationaryRECV/Third_Iteration/"+bitNumber+"Bit/RoomLight&Daylight/rawData/"+outputFileName+".csv", distance, bitNumber)
    # createCSVFile(data, "../../SignalDistanceTests/HandheldIRLEDStationaryRECV/Third_Iteration/"+bitNumber+"Bit/Daylight/rawData/"+outputFileName+".csv", distance, bitNumber)
    createCSVFile(data, "../data/"+outputFileName+".csv", distance, bitNumber)
                
decode()