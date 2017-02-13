#!/usr/bin/python

# Joshua Hesseltine
# CS372 Project 1
# 28 October 2016
#
# https://docs.python.org/2/howto/sockets.html
# http://stackoverflow.com/questions/1112343/how-do-i-capture-sigint-in-python
# https://www.tutorialspoint.com/python/python_networking.htm
# http://stackoverflow.com/questions/18704862/python-frame-parameter-of-signal-handler


import socket 
import sys 
import signal 

#per guidance, implment signal handler, used code from: http://stackoverflow.com/questions/18704862/python-frame-parameter-of-signal-handler
def signal_handler(signal, frame):
	print("")
	quit()
	
signal.signal(signal.SIGINT, signal_handler)

#confirming the correct number of args
if len(sys.argv) == 1:
	print("Correct: ChatClient [hostname] [port]\n")
	quit()
	
myHandle = raw_input("\nPlease enter a Server Handle: ")

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
host = socket.gethostname() 
portno = int(sys.argv[1]) 

sock.bind((host, portno)) 
sock.listen(5) 

while True: #while connection remains open 
	
	conn, addr = sock.accept() 
	
	stayConnected = True
	while stayConnected: #while exchange between client/server continues 

		
		client = conn.recv(10) 
		print("\nConnection established using port " + client + ".\n")
		
		keepAlive = True
		while keepAlive: #while buffered data is being recieved
			
			client = conn.recv(513) 
			
			if client == "\\quit":
				
				keepAlive = False
				stayConnected = False
				
			else: 
				
				print(client)
				server = raw_input(myHandle + "> ") 
				
				if server == "\\quit":
					
					completeServer = server
					keepAlive = False
					stayConnected = False
					
				else: 
					
					completeServer = myHandle + "> " + server
				
				conn.send(completeServer)

	conn.close() 
	