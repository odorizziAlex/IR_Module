import csv
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from pandas import DataFrame
import copy
import collections

class analyzer():
    def __init__(self, parent=None):
        #distance(cm)	start	1	2	3	4	5	6	7	8	9	10	11	12	13	14	15	16	17	18	19	20	21	22	23	24	25	26	27	28	29	30	31	32	end	index	code
        ## V1 | 32 Bit
        # self.referenceBit = "s11000001110001111100000000111111']" # 32 bit
        ## V2 | 16 Bit
        # self.referenceBit = "s1100000111000111t']" # 16 bit
        ## V2 | 32 Bit
        self.referenceBit = "s11000001110001111100000000111111t" # 32 bit
        # self.referenceBit = "s11000001110001111100000000111111t']" # 32 bit
        ## V2 | 64 Bit
        # self.referenceBit = "s1100011111000000001111111010101011111111101010101011101111001100t']" #64 bit

        self.fifty = []
        self.hunderdFifty = []
        self.threehunderd = []
        self.fiveHunderd = []


    def run(self):
        showPlot = True
        useOnlyCorrectSignals = False
        self.bitNumber = input("Number of bits: ")

        print("Show Only Correct Signals: ", useOnlyCorrectSignals)
        ## Test
        # pathRoomDaylightFile = './RoomLight&Daylight/SignalsRoomLight&Daylight.csv'
        ## V1
        # pathRoomDaylightFile = './SignalDistanceTests/HandheldIRLEDStationaryRECV/1.0_Iteration/'+self.bitNumber+'Bit/RoomLight&Daylight/SignalsRoomLight&DaylightV1.csv'
        ## V2:
        # pathRoomDaylightFile = './SignalDistanceTests/HandheldIRLEDStationaryRECV/1.5_Iteration/'+self.bitNumber+'Bit/RoomLight&Daylight/'+self.bitNumber+'BitRoomLight&DaylightV2.csv'
        ## V3.1 Test
        pathRoomDaylightFile = './ReceiveCustomSignal/data/TuningValues5.csv'
        ## V3.1
        # pathRoomDaylightFile = './SignalDistanceTests/HandheldIRLEDStationaryRECV/2.0_Iteration/'+self.bitNumber+'Bit/RoomLight&Daylight/'+self.bitNumber+'BitRoomLight&DaylightV3.csv'
        # pathRoomDaylightFile = './SignalDistanceTests/HandheldIRLEDStationaryRECV/2.0_Iteration/'+self.bitNumber+'Bit/RoomLight&Daylight/'+self.bitNumber+'BitRoomLight&DaylightV3.csv'


        ## Test
        # pathDaylightFile = './Daylight/SignalsDaylight.csv'
        ## V1
        # pathDaylightFile = './SignalDistanceTests/HandheldIRLEDStationaryRECV/First_Iteration/'+self.bitNumber+'Bit/Daylight/SignalsDaylightV1.csv'
        ## V2
        pathDaylightFile = './SignalDistanceTests/HandheldIRLEDStationaryRECV/Second_Iteration/'+self.bitNumber+'Bit/Daylight/'+self.bitNumber+'BitDaylightV2.csv'
        
        fileRoomDaylight = self.prepareData(pathRoomDaylightFile)
        # fileDaylight = self.prepareData(pathDaylightFile)
        # allData = self.prepareAllData(pathRoomDaylightFile, pathDaylightFile)

        print("Performance of AGC2 38kHz Receiver with 32bits in noisy environment:")

        ## CORRECTLY RECEIVED SIGNALS
        print("\n================== Correctly Received Signals Ratio ==================")
        print("\n================== Headlights and Daylight ==================")
        self.extractDataByDistance(fileRoomDaylight)
        self.checkHealthySignalNumber(useOnlyCorrectSignals)
        correctRoomDaylightData, startSignalRoomDaylightData, endSignalRoomDaylightData = self.extractData(fileRoomDaylight, useOnlyCorrectSignals)
        self.reset()
        # ## DURATION OF BITS
        print("\n================== Technical Data for tuning ==================")
        self.bitDuration(correctRoomDaylightData, "RoomLight and Daylight Signals | 1 and 0", showPlot)
        self.calculateMeanQuartile([startSignalRoomDaylightData],"RoomLight and Daylight STARTING Signals")
        self.calculateMeanQuartile([endSignalRoomDaylightData],"RoomLight and Daylight END Signals")
        # # ## CHECK SIGNAL HEALTH
        print("\n================== Signal Health ==================")
        self.signalHealth(fileRoomDaylight, "Headlights and Daylight | Health", showPlot)
        self.analyseMistakes(fileRoomDaylight, "Headlights and Daylight | Mistake analysis", showPlot)
        print("\n========================================================================")


        # ## CORRECTLY RECEIVED SIGNALS
        # print("\n================== Correctly Received Signals Ratio ==================")
        # print("\n================== Daylight ==================")
        # self.extractDataByDistance(fileDaylight)
        # self.checkHealthySignalNumber()
        # correctDaylightData, startSignalDaylightData, endSignalDaylightData = self.extractData(fileDaylight, True)
        # self.reset()
        # # ## DURATION OF BITS
        # print("\n================== Technical Data for tuning ==================")
        # self.bitDuration(correctDaylightData, "Daylight Signals | 1 and 0", showPlot)
        # self.calculateMeanQuartile([startSignalDaylightData],"Daylight STARTING Signals")
        # self.calculateMeanQuartile([endSignalDaylightData],"Daylight END Signals")
        # ## CHECK SIGNAL HEALTH
        # print("\n================== Signal Health ==================")
        # self.signalHealth(fileDaylight, "Daylight | Health", showPlot)
        # print("\n========================================================================")


        # ## CORRECTLY RECEIVED SIGNALS
        # print("\n================== Correctly Received Signals Ratio ==================")
        # print("\n================== All Data Combined ==================")
        # self.extractDataByDistance(allData)
        # self.checkHealthySignalNumber()
        # allCorrectSignals, startSignalAllData, endSignalAllData = self.extractData(allData, True)
        # self.reset()
        # # ## DURATION OF BITS
        # print("\n================== Technical Data for tuning ==================")
        # self.bitDuration(allCorrectSignals, "All Signals | 1 and 0", showPlot)
        # self.calculateMeanQuartile([startSignalAllData],"All STARTING Signals")
        # self.calculateMeanQuartile([endSignalAllData],"All END Signals")
        # ## CHECK SIGNAL HEALTH
        # print("\n================== Signal Health ==================")
        # self.signalHealth(allData, "All Signals | Health", showPlot)
        # print("\n========================================================================")

    def analyseMistakes(self, f, label, showPlot):
        file = copy.deepcopy(f)
    
        for index, line in enumerate(file):
            file[index].pop(0)
            file[index].pop(len(file[index])-2)

        oneMistakes = []
        zeroMistakes = []
        for line in file:
            seq = line[len(line)-1]
            for index, bit in enumerate(seq):


                if(bit == 'e'):
                    if(self.referenceBit[index] == '1'):
                        oneMistakes.append(int(line[index]))
                    elif(self.referenceBit[index] == '0'):
                        zeroMistakes.append(int(line[index]))
        print("\nQuantiles not recognized values")
        df1 = DataFrame(oneMistakes, columns=["oneMistakes"])
        df0 = DataFrame(zeroMistakes, columns=["zeroMistakes"])
        q1 = df1.oneMistakes.quantile([0,0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99,1])
        q0 = df0.zeroMistakes.quantile([0,0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99,1])
        print(q1)
        print(q0)
        
        if(showPlot):
            self.boxPlotData(zeroMistakes, label, "0")
            self.boxPlotData(oneMistakes, label, "1")
                

    def signalHealth(self, file, label, showPlot):
        print(label)

        sequenceList = []
        for i, line in enumerate(file):
            bitSequence = list(str(line[len(file[0])-1]))
            bitSequence.pop(0)
            bitSequence.pop(len(bitSequence)-1)
            bitSequence.pop(len(bitSequence)-1)
            sequenceList.append(bitSequence)

        # print(sequenceList)

        e = 0
        mistakeDict = {}
        for item in sequenceList:
            for bit in item:
                if(bit != '1' and bit != '0'):
                    e=e+1

            if e in mistakeDict:
                mistakeDict[e] = mistakeDict[e]+1
            
            if e not in mistakeDict:
                mistakeDict[e] = 1

            e = 0

        orderedMistakeDict = collections.OrderedDict(sorted(mistakeDict.items()))

        print("Mistake Number   :   Number of Signals  | Signal Health")
        for index in orderedMistakeDict:
            print(str(index)+"                :   "+str(orderedMistakeDict[index])+"                | "+str(100-int((index/len(sequenceList[0]))*100))+"%")

        keys = orderedMistakeDict.keys()
        xTicks = []
        for item in keys:
            xTicks.append(item)

        if(showPlot):
            keys = orderedMistakeDict.keys()
            values = orderedMistakeDict.values()
            fig, ax = plt.subplots()
            ax.set_title(label)
            ax.set_xlabel("number of mistakes")
            ax.set_ylabel("number of signals")
            ax.set_xticks(xTicks)
            ax.set_xticklabels(xTicks,rotation='vertical', fontsize=11)
            ax.bar(keys, values)
            plt.savefig(label)#, dpi=None, facecolor='w', edgecolor='w',orientation='portrait', papertype=None, format=None,transparent=False, box_inches=None, pad_inches=0.1,frameon=None, metadata=None)
            # plt.show()

    def prepareAllData(self, file1, file2):
        data = []
        with open(file1, mode = 'r') as csvFile:
            rawFile=csv.reader(csvFile)
            for i, row in enumerate(rawFile):
                if(i==0):
                    continue
                row = str(row).split(";")
                data.append(row)
        
        with open(file2, mode = 'r') as csvFile:
            rawFile=csv.reader(csvFile)
            for i, row in enumerate(rawFile):
                if(i==0):
                    continue
                row = str(row).split(";")
                data.append(row)
        
        return data

    def bitDuration(self, file, label, showPlot):
        dataOne = []
        dataZero = []
        referenceList = list(str(self.referenceBit))
        referenceList.pop(0)
        referenceList.pop(len(referenceList)-1)
        referenceList.pop(len(referenceList)-1)
        referenceList.pop(len(referenceList)-1)
        for line in file:
            for index,item in enumerate(referenceList):
                if(item == '1'):
                    dataOne.append(line[index])

                elif(item == '0'):
                    dataZero.append(line[index])
        if(showPlot):
            self.boxPlotData(dataOne, label, "1")
            self.boxPlotData(dataZero, label, "0")
        
        self.calculateMeanQuartile([dataOne, dataZero], label)
    
    def calculateMeanQuartile(self, data, label):
        print("\nMean and quartiles of "+label+": ")
        missingPointsNum = 0
        for item in data:
            mean = 0
            for p in item:
                if(int(p) != -1):
                    mean = mean+int(p)
                else:
                    missingPointsNum = missingPointsNum +1
            mean = mean/(len(item)-missingPointsNum)
            print("Mean duration: "+str(mean))

            df = DataFrame(item,columns=['OneBitDurations'])
            quantiles = df.OneBitDurations.quantile([0,0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99,1])
            print(quantiles)

    def prepareData(self, path):
        with open(path, mode='r') as csvFile:
            rawFile = csv.reader(csvFile)
            lines = []
            for i, row in enumerate(rawFile):
                if(i == 0):
                    continue
                row = str(row).split(";")
                lines.append(row)

        for index, l in enumerate(lines):
            if(len(l[int(self.bitNumber)+4]) > int(self.bitNumber)+2): ## 34
                lines[index][int(self.bitNumber)+4] = lines[index][int(self.bitNumber)+4][:-2]

        # print(len(lines[0]))
        # print(len(lines[201]))
        for i, l in enumerate(lines):
            if(len(l) == int(self.bitNumber)+6):
                lines[i].pop(int(self.bitNumber)+5)
                # print(i, l)
        # for i, l in enumerate(lines):
        #     print(i, len(l))
        return lines

    def extractDataByDistance(self, file):
        for i, v in enumerate(file):
            if(v[0] == "['50" ):
                self.fifty.append(v)
            elif(v[0] == "['150" ):
                self.hunderdFifty.append(v)
            elif(v[0] == "['300" ):
                self.threehunderd.append(v)
            elif(v[0] == "['500" ):
                self.fiveHunderd.append(v)
    
    def extractData(self, f, onlyCorrect):
        file = copy.deepcopy(f)
        data = []
        starterSignals = []
        endSignals = []
        counter = 0

        if(onlyCorrect):
            for row in file:

                if(row[int(self.bitNumber)+4] == self.referenceBit): #36
                    data.append(row)
                    counter = counter+1

            print("Total: "+str(len(file))+" tries, "+str(counter)+" signals are correctly received. "+ str(counter/len(file)))
        else: 
            data = file
            counter = len(data)

        i = 0
        for i,line in enumerate(data):

            if(len(line) >= 0):
                line.pop(0)

            if(len(line) >= 1):
                starterSignals.append(int(line[0]))
                line.pop(0)

            if(len(line) >= int(self.bitNumber)+2): # 34
                line.pop(int(self.bitNumber)+2)

            if(len(line) >= int(self.bitNumber)+1): # 33
                line.pop(int(self.bitNumber)+1)

            if(len(line) >= int(self.bitNumber)): # 32
                if(line[int(self.bitNumber)] == ''):
                    endSignals.append(-1)
                else:
                    endSignals.append(int(line[int(self.bitNumber)]))
                
                line.pop(int(self.bitNumber))

            for j,item in enumerate(line):
                data[i][j] = int(item)

        return data, starterSignals, endSignals

    def checkHealthySignalNumber(self, onlyCorrect):
        counterFifty = 0
        for row in self.fifty:
            if(row[int(self.bitNumber)+4] == self.referenceBit):
                counterFifty = counterFifty + 1
        print("Distance: 50cm: "+str(len(self.fifty))+" tries, "+str(counterFifty)+" signals are correctly received. "+ str(counterFifty/len(self.fifty)))
        
        counterHundredfifty = 0
        for row in self.hunderdFifty:
            if(row[int(self.bitNumber)+4] == self.referenceBit):
                counterHundredfifty = counterHundredfifty + 1
        print("Distance: 150cm: "+str(len(self.hunderdFifty))+" tries, "+str(counterHundredfifty)+" signals are correctly received. "+ str(counterHundredfifty/len(self.hunderdFifty)))
        
        counterThreehundred = 0
        for row in self.threehunderd:
            if(row[int(self.bitNumber)+4] == self.referenceBit):
                counterThreehundred = counterThreehundred + 1
        print("Distance: 300cm: "+str(len(self.threehunderd))+" tries, "+str(counterThreehundred)+" signals are correctly received. "+ str(counterThreehundred/len(self.threehunderd)))
        
        counterFivehundred = 0
        for row in self.fiveHunderd:
            if(row[int(self.bitNumber)+4] == self.referenceBit):
                counterFivehundred = counterFivehundred + 1
        print("Distance: 500cm: "+str(len(self.fiveHunderd))+" tries, "+str(counterFivehundred)+" signals are correctly received. "+ str(counterFivehundred/len(self.fiveHunderd)))

        if(not onlyCorrect):
            totalDocLength = len(self.fifty)+len(self.hunderdFifty)+len(self.threehunderd)+len(self.fiveHunderd)
            correctSignalNumer = counterFifty+counterHundredfifty+counterThreehundred+counterFivehundred
            print("Total: "+str(totalDocLength)+" tries, "+str(correctSignalNumer)+" signals are correctly received. "+ str(correctSignalNumer/totalDocLength))

    def reset(self):
        self.fifty = []
        self.hunderdFifty = []
        self.threehunderd = []
        self.fiveHunderd = []
        
    def boxPlotData(self, data, label, bit):
        ##https://matplotlib.org/3.1.1/gallery/pyplots/boxplot_demo_pyplot.html
        fig1, ax1 = plt.subplots()
        ax1.set_title(label+str(" | Logical ")+bit+str("s "))
        # ax1.set_xlabel("")
        ax1.set_ylabel("signal duration in microseconds")
        ax1.boxplot(data)
        plt.savefig(label+str(" | Logical ")+bit+str("s "))#, dpi=None, facecolor='w', edgecolor='w',orientation='portrait', papertype=None, format=None,transparent=False, box_inches=None, pad_inches=0.1,frameon=None, metadata=None)
        # plt.show()


a = analyzer()
a.run()