# Joshua Hesseltine
# CS372 -- Project2
# 11/23/2016
# ftclient.py

#resources
#https://docs.python.org/3.4/howto/sockets.html
#https://pymotw.com/2/socket/tcp.html
#https://www.tutorialspoint.com/python/python_networking.htm
#http://stackoverflow.com/questions/33018079/tcp-socket-programming-in-python-login-ftp

import os                      
import re                       
import sys                      
from socket import (socket,gethostbyname,AF_INET,SOCK_STREAM,SOL_SOCKET,SO_REUSEADDR)
from struct import pack, unpack 

def main():
    global serverHost
    global serverPort
    global parameter
    global filename
    global dataPort

    if len(sys.argv) not in (5, 6):
        print "python2 ftclient <server> <port> -l|-g [<filename>] <port>"
        sys.exit(1)
    serverHost = gethostbyname(sys.argv[1])
    serverPort = sys.argv[2]
    parameter = sys.argv[3]
    filename = sys.argv[4] if len(sys.argv) == 6 else None
    dataPort = sys.argv[5] if len(sys.argv) == 6 else sys.argv[4]

    # check for filename argument
    if parameter == "-g" and filename is None:
        print "python2 ftclient <server> <port> -l|-g [<filename>] <port>"
        sys.exit(1)

    # check type input
    if not validatePort(serverPort):
        print "port type not valid -- must be an interget"
        sys.exit(1)
    serverPort = int(serverPort)

    # check range
    if int(serverPort) < 1024 or int(serverPort) > 65535:
        print "port must be in the range [1024, 65535]"
        sys.exit(1)

    # check argument param
    if parameter not in ("-l", "-g"):
        print "please enter eithier -l or -g"
        sys.exit(1)

    # check type again on data port
    if not validatePort(dataPort):
        print "port type not valid -- must be an interget"
        sys.exit(1)
    dataPort = int(dataPort)

    # check range on dataPort
    if int(dataPort) < 1024 or int(dataPort) > 65535:
        print "port must be in the range [1024, 65535]"
        sys.exit(1)

    # check data port is deffirent from server port
    if serverPort == dataPort:
        print "the server port and data port must be different"
        sys.exit(1)

    createFTP()
    sys.exit(0)

# function to validate port number is an integer value
#source: http://stackoverflow.com/questions/2838244/get-open-tcp-port-in-python
def validatePort(string):
    return re.match("^[0-9]+$", string) is not None

# establish connection and process data
#resource: https://pymotw.com/2/socket/tcp.html
def createFTP():
    try:
        controlSocket = socket(AF_INET, SOCK_STREAM, 0)
    except Exception as e:
        print e.strerror
        sys.exit(1)
        
    try:
        controlSocket.connect((serverHost, serverPort))
    except Exception as e:
        print e.strerror
        sys.exit(1)
    print "control connection established with server"   
    print "data socket established"
    outtag = "DPORT"
    outdata = str(dataPort)
    sendPacket(controlSocket, outtag, outdata)
    outtag = "NULL"
    outdata = ""
    if parameter == "-l":
        outtag = "LIST"
    elif parameter == "-g":
        outtag = "GET"
        outdata = filename
    sendPacket(controlSocket, outtag, outdata)
    intag, indata = validatePacket(controlSocket)
    status = 0
    
    if status != -1:
        try:
            clientSocket = socket(AF_INET, SOCK_STREAM, 0)
        except Exception as e:
            print e.strerror
            sys.exit(1)
            
        try:
            clientSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
            clientSocket.bind(("", dataPort))
        except Exception as e:
            print e.strerror
            sys.exit(1)
            
        try:
            clientSocket.listen(10)
        except Exception as e:
            print e.strerror
            sys.exit(1)
            
        try:
            dataSocket = clientSocket.accept()[0]
        except Exception as e:
            print e.strerror
            sys.exit(1)
        print "data connection created with server"                   

        createDataConnection(controlSocket, dataSocket)
        while True:
            intag, indata = validatePacket(controlSocket)
            if intag == "ERROR":
                print " " + indata
            if intag == "CLOSE":
                break

    controlSocket.close()
    print "control-connection closed"
    
#source: https://pymotw.com/2/socket/tcp.html
def sendPacket(socket, tag = "", data = ""):
    packetLength = 2 + 8 + len(data)
    packet = pack(">H", packetLength)
    packet += tag.ljust(8, "\0")
    packet += data
    try:
        socket.sendall(packet)
    except Exception as e:
        print e.strerror
        sys.exit(1)
        
#source: https://pymotw.com/2/socket/tcp.html      
def validatePacket(socket):
    packetLength = unpack(">H", checkFilename(socket, 2))[0]
    tag = checkFilename(socket, 8).rstrip("\0")
    data = checkFilename(socket, packetLength - 8 - 2)
    return tag, data
    
def checkFilename(socket, numBytes):
    data = "";
    while len(data) < numBytes:
        try:
            data += socket.recv(numBytes - len(data))
        except Exception as e:
            print e.strerror
            sys.exit(1);
    return data

def createDataConnection(controlSocket, dataSocket):
    ret = 0 
    intag, indata = validatePacket(dataSocket)

    if intag == "FNAME":
        print "Directory contents: "

        # Print all received filenames.
        while intag != "DONE":
            print "  " + indata
            intag, indata = validatePacket(dataSocket)

    elif intag == "FILE":
        filename = indata
        if os.path.exists(filename):
           print "file \"{0}\" already exists".format(filename)
           ret = -1
           
        else:
            with open(filename, "w") as outfile:
                while intag != "DONE":
                    intag, indata = validatePacket(dataSocket)
                    outfile.write(indata)
            print "completed file transfer"
    else:
        ret = -1
    sendPacket(controlSocket, "ACK", "")
    return ret
    
main()
