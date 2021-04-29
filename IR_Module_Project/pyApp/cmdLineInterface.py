from __future__ import print_function
from AppKit import NSWorkspace
from Foundation import *
from os import system
from PIL import Image #Pillow
from Quartz import CGWindowListCopyWindowInfo, kCGNullWindowID, kCGWindowListOptionAll
from os import listdir
from os.path import isfile,join
import appscript
import time
import pyautogui as pag
import sys
import os
import pyrebase
import subprocess
import objc 
import serial
import glob
import serial
import serial.tools.list_ports as lp
import glob
import threading
import random
import sys
import webbrowser
import readline
import requests

class WifiHandler:

    #ENDPOINT
    #Source https://www.youtube.com/watch?v=2Fp1N6dof0Y
    # Under the use of 'subprocess' Module, the command wifiPassword is executed.
    # If needed dependencies are installed, this returns wifi name and password, in case the user is 
    # connected. It also requires admin permission first.
    def getWifiData(self):
        wifiData = subprocess.run("wifiPassword", capture_output = True)
        wifiName, wifiPassword = self.stripWifiData(str(wifiData.stdout))
        return "<"+wifiName+">"+"<"+wifiPassword+">"
    
    def connectToWifi(self, data):
        wifiData = self.cleanWifiData(data)
        name = wifiData[0]
        password = wifiData[1]
        objc.loadBundle("CoreWLAN", bundle_path = '/System/Library/Frameworks/CoreWLAN.framework', module_globals = globals())
        iface = CWInterface.interface()
        networks, error = iface.scanForNetworksWithName_error_(name, None)
        network = networks.anyObject()
        success, error = iface.associateToNetwork_password_error_(network, password, None)

    def cleanWifiData(self, data):
        return [data.split(">")[0][1:], data.split(">")[1][1:]]

    def stripWifiData(self, rawWifiData):
        rawWifiData = rawWifiData.split("'")
        rawWifiData = rawWifiData[1][:-1]
        rawWifiData = rawWifiData.split(" ")
        wifiName = rawWifiData[len(rawWifiData)-2][:-1]
        wifiPassword = rawWifiData[len(rawWifiData)-1][:-1]
        return wifiName, wifiPassword

class DBHandler:
    def __init__(self,parent=None):
        self.stopInternetThread = False
        self.startInternetConnectionThread()

    def startInternetConnectionThread(self):
        internetThread = ThreadHandler(3,"internet_connection",self)
        internetThread.start()

    def awaitInternetConnection(self):
        while not self.stopInternetThread:
            time.sleep(1)
            try:
                req = requests.get("https://google.com/", timeout=5)
                self.stopInternetThread = True
                self.initFirebaseCloud()
            except:
                pass

    # Downloads image from database. After downloading, the image will be deleted from the database.
    def downloadImage(self):
        toggle = True
        imgCounter = 0
        while toggle:
            try:
                imgCounter = imgCounter +1
                self.storage.child("images/activeWindowScreenshot"+str(imgCounter)+".png").download("activeWindowScreenshot"+str(imgCounter)+".png")
                self.storage.delete("images/activeWindowScreenshot"+str(imgCounter)+".png")
            except:
                toggle = False

    # Uploads image to database
    def uploadImage(self):
        onlyfiles = [f for f in listdir("./imageStorage/images/") if isfile(join("./imageStorage/images/", f))]
        for imgFile in onlyfiles:
            if str(imgFile) != ".DS_Store":
                self.storage.child("images/"+str(imgFile)).put("./imageStorage/images/"+str(imgFile))
                os.remove("./imageStorage/images/"+str(imgFile))
    
    # Initiates database
    def initFirebaseCloud(self):
        config = {
            "apiKey": "AIzaSyCOvch1q5uHL_pdZ6pfClAcIy7MBNHWdD8",
            "authDomain": "imagestorage-302213.firebaseapp.com",
            "databaseURL": "imagestorage-302213.appspot.com",
            "projectId": "imagestorage-302213",
            "storageBucket": "imagestorage-302213.appspot.com",
            "messagingSenderId": "808913976749",
            "appId": "1:808913976749:web:478c59509709cc135c3524",
            "serviceAccount": "./../../IR_ModuleAssets/imagestorage-302213-firebase-adminsdk-sztpo-ada97df2b7.json",
            "measurementId": "G-MLWT8NZ2MG"
        }
        self.imgName = "activeWindowScreenshot.png"
        firebase = pyrebase.initialize_app(config)
        self.storage = firebase.storage()
        self.path_on_cloud = "images/"+self.imgName
        self.path_local = "./imageStorage/images/"+self.imgName
        print("<< Online cloud connected.")

# Functionality of this class is based on Kalle Halldens project called "AutoTimer".
# Source: https://github.com/KalleHallden/AutoTimer
# This class is responsible for capturing the state of the focussed application.
class Capturer:
    
    # This function identifies the application which currently is in focus of the system.
    # Different applications need to be treated differently when it comes to capturing their current state.
    def getCurrentlyActiveTask(self):
        activeWindowName = ""
        data = None
        newWindowName = NSWorkspace.sharedWorkspace().activeApplication()['NSApplicationName']
        if activeWindowName != newWindowName:
            activeWindowName = newWindowName
            if activeWindowName == "Google Chrome" or activeWindowName == "Safari":
                data = self.getURLFromSupportedBrowser(activeWindowName)
                if str(type(data)) == "<class 'str'>":
                    data = self.cleanGoogleQueryURLs(data)
            else:
                data = self.captureWindow(activeWindowName)

        return data

    # Google URLs can be very long. They need to be shortened.
    def cleanGoogleQueryURLs(self,url):
        if 'http://www.google.' in url or 'https://www.google.' in url:
            url = url.split('&')
        else:
            return url
        return url[0]

    # To pass the currently open browser content, the URL needs to be extracted from the browser.
    # It Google Chrome, or Safari are used, the URL can be extracted.
    def getURLFromSupportedBrowser(self,browserName):
        url = ""
        if browserName == "Google Chrome":
            textOfMyScript = """tell app "google chrome" to get the url of the active tab of window 1"""
            s = NSAppleScript.initWithSource_(NSAppleScript.alloc(), textOfMyScript)
            results, err = s.executeAndReturnError_(None)
            if err != None and results == None:
                print("<< Error: Extracting URL from browser. Take screenshot instead.")
            else:    
                return str(results.stringValue())
        elif browserName == "Safari":
            try:
                currentURLSafari = appscript.app('Safari').windows.first.current_tab.URL()
                return str(currentURLSafari)
            except:
                print("<< Error: Extracting URL from browser. Take screenshot instead.")
        
        return self.captureWindow(browserName)

    def captureWindow(self,windowName):
        windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID)
        images = []
        index = 0
        referenceType = None
        for window in windowList:
            if 'kCGWindowOwnerName' in window and 'kCGWindowIsOnscreen' in window:
                if windowName.lower() == window['kCGWindowOwnerName'].lower() and window['kCGWindowIsOnscreen'] == 1:
                    if 'kCGWindowName' in window and window['kCGWindowName'] == "":
                        continue

                    elif 'kCGWindowName' not in window:
                        print("<< WARNING: Capturer.captureWindow() could not find 'kCGWindowName' in CGWindowListCopyWindowInfo of "+windowName+" window. Capture could be corrupted.")

                    index+=1
                    winBounds = window['kCGWindowBounds']                    
                    # Use every value times 2 to match dimensions of the actual application window
                    image = pag.screenshot(region=(2*(winBounds["X"]),2*(winBounds["Y"]),2*(winBounds["Width"]),2*(winBounds["Height"])))
                    images.append(image)
                    for img in images: 
                        img.save("./imageStorage/images/activeWindowScreenshot"+str(index)+".png")
                        referenceType = img
        if(referenceType == None):
            return "https://google.com/search?q=IR+Module+Capturer+Error:+Unable+capture+window+of+other+computer..."
            
        return referenceType


    def openURL(self, url):
        webbrowser.get().open(url)

# Source: https://www.tutorialspoint.com/python/python_multithreading.htm
# This class is responsible for running two threads at the same time.
# 1. Listening for incoming commands from the IR Module.
# 2. Listening for user Input to commandline.
class ThreadHandler(threading.Thread):
    def __init__(self, threadID, name, superClass):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.superClass = superClass

    def run(self):
        if self.threadID == 1:
            self.superClass.stopSerialThread = False
            self.superClass.awaitIncomingIRData()
        elif self.threadID == 2:
            self.superClass.stopInputThread = False
            self.superClass.listenForUserInput()
        elif self.threadID == 3:
            self.superClass.awaitInternetConnection()
        else: 
            raise Exception("Thread Handler: Thread Id not found:",self.threadID,"name:",self.name)

class main():
    def __init__(self,parent=None):
        self.cap = Capturer()
        self.wif = WifiHandler()
        self.db = DBHandler()

        # START IR Module communication variables 
        self.maxByteNumber = 251 # 765 would be the actual capacity of the IR Module. readBytesUntil() wont take more than 251 though...
        self.authorizationValue = 187
        self.moduleIncomingPayloadCommand = 'S'
        self.moduleDataReceptionErrorCommand = 'E'
        self.modulePayloadReceptionCommand = 'D'
        self.moduleTransmissionSuccessCommand = 'X'
        self.moduleResetCommand = 'R'
        self.moduleHardResetCommand = 'H'
        self.moduleSerialPortConnection = None
        self.serialBaudRate = 115200
        # END IR Module communication variables 

        # START Command line interface commands
        self.exitCmdLineAppCommand = "exit()"
        self.manualResetIRModuleCmdLineCommand = "reset()"
        self.activityRequestInterfaceCommand = "act"
        self.wifiRequestInterfaceCommand = "wif"
        self.makeDBRequestInterfaceCommand = "uDB"
        # END Command line interface commands
        
        # START Module IDs
        self.moduleIdD1MiniPro = 'CP2104'
        self.moduleIdD1Mini = 'USB2.0-Serial'
        # END Module IDs

        # START Thread instances
        self.responseThread = None
        self.userInputThread = None
        # END Thread instances

        # START Thread triggers
        self.stopInputThread = False
        self.stopSerialThread = False
        self.outputRequest = None
        # END Thread triggers
    
    # Runs authorization protocol to check if IR Module is ready to run.
    def authorizeModule(self, port):
        # Generates a Number, which needs to be returned by the IR Module.
        # This indicates, that the right IR Module is connected.
        randomInteger = random.randint(0,255)
        authorizationCode = randomInteger.to_bytes(1, "big")
        expectedResponse = randomInteger ^ self.authorizationValue
        # Open serial connection, pass authorization code and check response.
        try:
            with serial.Serial(port, self.serialBaudRate, timeout=1) as ser:
                ser.write(authorizationCode)
                time.sleep(.1)
                response = self.cleanSerialInput(ser.readline())
                if response == '':
                    ser.close()
                    return False
                else:
                    response = int(response)
                if expectedResponse == response:
                    time.sleep(.1)
                    ser.write(self.authorizationValue.to_bytes(1,"big"))
                    ser.close()
                    return True
            ser.close()
        except:
            print("Module connection Error: Authorization on port:",port)
            return False

        return False

    # Scanning ports for Module IDs and check if port response matches expected IR Module response.
    # If response is as expected, IR Module is ready to receive commands.
    def findAvailablePorts(self):
        tempPort = None
        availablePorts = list(serial.tools.list_ports.comports())
        if availablePorts != []:
            for port in availablePorts:
                if self.moduleIdD1MiniPro in port.description or self.moduleIdD1Mini in port.description:
                    tempPort = str(port.device)
                    # Runs authorization protocol and check if IR Module is ready to run.
                    if self.authorizeModule(tempPort):
                        self.moduleSerialPortConnection = serial.Serial(tempPort, self.serialBaudRate, timeout=5)
                        print("<< IR Module found on port:",tempPort)
                        return True
        return False

    def findModule(self):
        if self.findAvailablePorts() == False:
            raise Exception("Commandline Interface: No IR Module found (Try IR Module Reset)")
            sys.exit()

    def cleanSerialInput(self, data):
        data = str(data)
        cleanInput = data.split("'")[1]
        cleanInput = cleanInput[:-4]
        cleanInput = cleanInput.split("\\")
        return cleanInput[0]

    def serialWrite(self, data):
        self.moduleSerialPortConnection.write(bytes(str(data).encode('ascii')))

    # This function takes care of the data passing protocol between PC and IR Module
    def passDataToModule(self, data):
        dataString = str(data)
        # Check data size.
        inputByteNumber = len(str(dataString).encode('ascii'))
        if inputByteNumber > self.maxByteNumber or inputByteNumber == 0:
            print("<< Error: Byte number error. Byte number must be at least 1 or 765 at most.",str(inputByteNumber),str(data))
            return 
        # Here the data passing protocol starts:
        # Step 1: Pass total number of bytes of the following payloads and read the IR Modules response.
        self.serialWrite(self.moduleIncomingPayloadCommand+str(inputByteNumber))
        time.sleep(.01)
        response = self.cleanSerialInput(self.moduleSerialPortConnection.readline())
        if response == self.moduleIncomingPayloadCommand:
            # Step 2: Pass data as bytes and read the IR Modules response.
            self.serialWrite(dataString)
            response = self.cleanSerialInput(self.moduleSerialPortConnection.readline())
            if response == self.moduleDataReceptionErrorCommand:
                raise Exception("Commandline Interface: Data reception error. Raised by IR Module.")
            elif response == self.modulePayloadReceptionCommand:
                print("<< Data passed successfully")
                return
            else: 
                raise Exception("Commandline Interface: IR Module to host communication failed. Module Response unknown after passing payload:",response)
        # Sometimes, the IR Module resets itself. Most of the time this code is returned.
        elif response == '232':
                raise Exception("Commandline Interface: IR Module unwanted reset after serial connection. Module response: "+str(response))
        else:
            print("<< Error: IR Module to host communication failed. Module Response unknown after passing total byte number :",response,"\n<< Type: reset()")
            self.serialWrite(self.moduleResetCommand)

    # This function differentiates between datatypes, the Capturer class as returned.
    # Based on the data type, different actions need to be triggered
    def processCapturedData(self):
        capturedData = self.cap.getCurrentlyActiveTask()
        print("<< Captured data:",capturedData)
        if str(type(capturedData)) == "<class 'PIL.Image.Image'>":
            self.db.uploadImage()
            return self.makeDBRequestInterfaceCommand
        elif str(type(capturedData)) == "<class 'str'>" and capturedData != '':
            return capturedData
        elif str(type(capturedData)) == "<class 'str'>" and capturedData == '':
            print("Error: Unable to extract URL")
            return False
        else:
            print("Error: Nothing was captured")
            return False
    
    # Interrupt user input to free admin password input
    def interruptInput(self):
        self.stopInputThread = True
        print("<< Admin Password is needed. Press ENTER to continue.")
        self.userInputThread.join()
        self.userInputThread = None
        return True

    # Scanning for incoming interface commands to trigger specific functions.
    # If no interface commands are recognized, put out the incoming value as a string.
    # Incoming commands are split in two types:
    #   1. Commands which entail a response and
    #   2. Commands which entail an IR Module reset.
    def processIncomingData(self, command):
        data = None
        # Type 1 commands: Trigger data capturing followed by uploading to the IR Module.
        if command == self.wifiRequestInterfaceCommand:
            print("<< WIFI request received. Loading...")
            # Interrupt user input because admin password will be needed.
            if self.interruptInput():
                data = self.wif.getWifiData()
                self.upload(data)
            print()
        elif command == self.activityRequestInterfaceCommand:
            # Captures current Activity.
            print("<< ACTIVITY request received. Loading...\n")
            data = self.processCapturedData()
            if data == False:
                self.initListener()
                return
            self.upload(data)
        # Type 2 commands: Trigger action based on incoming commands.
        elif command == self.makeDBRequestInterfaceCommand:
            # Triggers database image download.
            print("<< DATABASE command received. Downloading...\n")
            self.outputRequest = ""
            self.db.downloadImage()
            self.serialWrite(self.moduleResetCommand)
            self.initListener()
        elif self.outputRequest == self.activityRequestInterfaceCommand and command != self.makeDBRequestInterfaceCommand:
            # Triggers opening of received URL.
            print("<< URL received. Opening...\n")
            self.outputRequest = ""
            self.cap.openURL(command)
            self.serialWrite(self.moduleResetCommand)
            self.initListener()
        elif self.outputRequest == self.wifiRequestInterfaceCommand:
            # Triggers login to WIFI network with received data.
            print("<< WIFI data received. ESTABLISHING CONNECTION...\n")
            self.outputRequest = ""
            self.wif.connectToWifi(command)
            self.serialWrite(self.moduleResetCommand)
            self.initListener()
        else:
            # Prints incoming Message
            print()
            print("=========================================")
            print("<< MESSAGE RECEIVED:\n"+str(command))
            print("=========================================")
            print()
            self.serialWrite(self.moduleResetCommand)
            self.initListener()

    # Listening for data incoming commands by the IR Module.
    def awaitIncomingIRData(self):
        print("<< Ready for incomming IR commands.")
        while not self.stopSerialThread:
            serialInput = self.cleanSerialInput(self.moduleSerialPortConnection.readline())
            # Only if the module indicates a successful reception of data, the serial input will be processed.
            if serialInput == self.moduleTransmissionSuccessCommand:
                print("<< Data Received! Loading...")
                time.sleep(.01)
                serialInput = self.cleanSerialInput(self.moduleSerialPortConnection.readline())
                self.stopSerialThread = True
                self.processIncomingData(serialInput)
                break 
    
    # Initiates command line input thread
    def initUserInput(self):
        self.userInputThread = ThreadHandler(2,"user_input_thread",self)
        self.userInputThread.start()

    # User input listener, called by user input thread, to enable user input any time.
    def listenForUserInput(self):
        while not self.stopInputThread:
            userInput = input("<< Ready for user input.\n")
            print()
            if userInput == "":
                continue
            elif userInput == self.exitCmdLineAppCommand:
                print("Exiting...")
                self.serialWrite(self.moduleHardResetCommand)
                self.stopSerialThread = True
                self.stopInputThread = True
                self.db.stopInternetThread = True
                sys.exit()
            elif userInput == self.manualResetIRModuleCmdLineCommand:
                self.serialWrite(self.moduleResetCommand)
            else:
                self.stopSerialThread = True
                self.waitForSerialThreadClose()
                self.upload(userInput)

    def waitForSerialThreadClose(self):
        if self.responseThread != None:
            print("<< Loading...")
            self.responseThread.join()
            self.responseThread = None

    def inspectContent(self,content):
        self.outputRequest = ""
        if content == "act()":
            self.outputRequest = self.activityRequestInterfaceCommand
            content = self.activityRequestInterfaceCommand
        elif content == "wif()":
            self.outputRequest = self.wifiRequestInterfaceCommand
            content = self.wifiRequestInterfaceCommand
        return content

    # Initiates listener threads to listen for incoming IR commands or commandline commands.
    # This can be called from command line.
    def initListener(self):
        self.responseThread = ThreadHandler(1,"serial_thread",self)
        self.responseThread.start()
        if self.userInputThread == None:
            self.initUserInput()

    # This can be called from command line to immediately upload data to the IR Module.
    def upload(self, content):
        # Caches known commands, which trigger an automatic response.
        content = self.inspectContent(content)
        self.passDataToModule(content)
        # Initiates listener threads to listen for incoming IR commands or command line commands.
        self.initListener()

IRInterface = main()
manual = [
    "WELCOME TO THE COMMANDLINE INTERFACE FOR THE IR MODULE.",
    "(This program is optimized to run on MacOS Big Sur (11.2.3))",
    "Make sure the IR Module is connected and in observation mode (For double check press reset on ESP8266) before starting this program.",
    "You can enter commands while the system is running. (Submit with Return-Key)",
    "any message:   Type a message which should be sent via IR.",
    IRInterface.manualResetIRModuleCmdLineCommand+":       Clears input buffer of the module. Also useful to check module readiness.",
    IRInterface.activityRequestInterfaceCommand+"():          Send an 'Activity Request' to receive whatever the other host PC has in focus.",
    "               This can be either a screenshot which will be downloaded to this directory, or an URL.",
    IRInterface.wifiRequestInterfaceCommand+"():          Send a Request for the Wifi connection, the other PC is connected to.",
    IRInterface.exitCmdLineAppCommand+":        To close this program and completely reset the IR Module.",
    "",
    ]

args = sys.argv
if len(args) == 3 and args[1] == "upload":
    IRInterface.findModule()
    IRInterface.upload(args[2])
elif len(args) == 2 and args[1] == "listener":
    IRInterface.findModule()
    IRInterface.initListener()
elif len(args) > 3:
    print("<< Too many arguments...\n<< Try: python cmdLineInterface.py upload 'Hello World'")
else:
    print("==========================")
    for line in manual:
        print("<< "+line)
    print("==========================")
    IRInterface.findModule()
    IRInterface.initListener()

