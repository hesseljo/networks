Joshua Hesseltine
CS372 -- Project2
11/23/2016

Background:
File transfer program using TCP socket connection to implement
file transfer client-server application 
Two arguments are taken via command line -l||-g
	-l: list directory request to server port where ftserver is running, connection is
			established and directory list is displayed
	-g: connection is established with "FILENAME.txt" argument supplied and transferred to server
	on unique data port. 

Instructions:

Always Run Server first.

	**ON HOST A**
	copy ftserver.c onto host A (e.g. flip1)
	compile ftserver.c by typing "gcc ftserver.c -o ftserver -Wall"

	RUN: to start ftserver type "ftserver [SERVERPORTNUMBER]"
	EXAMPLE: "ftserver 30021"
	
	
	**ON HOST B**
	copy ftclient.py to host B (e.g. flip2)
	copy longfile.txt and shortfile.txt to host b
	
	RUN argument for list "-l":
		python [SERVER] [SERVERPORT] -l [DATAPORT]
	EXAMPLE:	"ftclient.py flip1 30021 -l 30022"
	
	RUN argument for file transfer -g:
		python [SERVER] [SERVERPORT] -l [FILENAME.txt] [DATAPORT] 
	EXAMPLE:	"ftclient.py flip1 30021 -l longfile.txt 30022"


End Connection:
RUN: type CTRL-C in server console window
