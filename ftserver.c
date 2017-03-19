/*
* Joshua Hesseltine
* CS372 -- Project2
* 11/23/2016
* ftserver.c
*
*resources:
*http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpclient.c
*http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/server.c
*http://web.cecs.pdx.edu/~jrb/tcpip/sockets/ipv6.src/tcp/tcpserver.c
*https://www.cs.rutgers.edu/~pxk/rutgers/notes/sockets/
*http://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner
*http://stackoverflow.com/questions/17728050/second-signal-call-in-sighandler-what-for
*
*
*/

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

void SigHandler(int sig);
void createFTP(int port);
char **listFiles(char *directory, int *numFiles);
void incomingFile(int socket, void *buffer, int size);
void checkPacket(int socket, char *tag, char *data);
void sendFile(int socket, void *buffer, int numBytes);
void sendPacket(int socket, char *tag, char *data);

int validatePort(char *str, int *n);
int setConnection(int controlSocket, char *commandTag, int *dataPort, char* filename);
int createDataConnection(int controlSocket, int dataSocket, char *commandTag, char *filename);


int main(int argc, char **argv)
{
	int port;  
	if (argc != 2) {
		fprintf(stderr, "<server-port>\n");
		exit(1);
	}
	if (!validatePort(argv[1], &port)) {
		fprintf(stderr, "not valid port type\n");
		exit(1);
	}
	if (port < 1024 || port > 65535) {
		fprintf(stderr, "port must be in the range[1024, 65535]\n");
		exit(1);
	}
	createFTP(port);
	exit(0);
}

//source: http://stackoverflow.com/questions/17728050/second-signal-call-in-sighandler-what-for
void SigHandler(int sig)
{
	int status;                  
	struct sigaction interrupt;   
	printf("\nnot accepting connections...\n");

	interrupt.sa_handler = SIG_DFL;
	status = sigaction(SIGINT, &interrupt, 0);
	if (status == -1) {
		perror("sigaction");
		exit(1);
	}
	status = raise(SIGINT);
	if (status == -1) {
		perror("raise");
		exit(1);
	}
}

void createFTP(int port)
{	

	int serverSocket;             
	int status;                    
	struct sigaction interrupt;       
	struct sockaddr_in serverAddress; 
	serverAddress.sin_family = AF_INET;       
	serverAddress.sin_port = htons(port);      
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	// Create server-side socket.
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1) {
		perror("socket");
		exit(1);
	}
	status = bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
	if (status == -1) {
		perror("bind");
		exit(1);
	}
	status = listen(serverSocket, 5);
	if (status == -1) {
		perror("listen");
		exit(1);
	}

	interrupt.sa_handler = &SigHandler;
	interrupt.sa_flags = 0;
	sigemptyset(&interrupt.sa_mask);
	status = sigaction(SIGINT, &interrupt, 0);
	if (status == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server open on port %d\n", port);
	while (1) {               
		char commandTag[8 + 1];      
		char filename[512 + 1]; 
		int controlSocket, dataSocket;     
		int dataPort;                    
		socklen_t addrLen;              
		struct sockaddr_in clientAddress;  

		addrLen = sizeof(struct sockaddr_in);
		controlSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &addrLen);
		if (controlSocket == -1) {
			perror("accept");
			exit(1);
		}
	
		printf("\ncontrol connection established with client");
		status = setConnection(controlSocket, commandTag, &dataPort, filename);
		if (status != -1) {
			int tries; 
			dataSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (dataSocket == -1) {
				perror("socket");
				exit(1);
			}
			
			clientAddress.sin_port = htons(dataPort);
			tries = 0;
			do 
			{
				status = connect(dataSocket, (struct sockaddr *) &clientAddress, sizeof(clientAddress));
			} while (status == -1 && tries < 12);
			if (status == -1) 
			{
				perror("connect");
				exit(1);
			}
			printf("\ndata connection established with client");

			createDataConnection(controlSocket, dataSocket, commandTag, filename);
			checkPacket(controlSocket, NULL, NULL);
			status = close(dataSocket);
			if (status == -1) {
				perror("close");
				exit(1);
			}
			printf("data connection closed\n");
		}
	}
}

int validatePort(char *str, int *n)
{
	char c;
	int matches = sscanf(str, "%d %c", n, &c);
	return matches == 1;
}

//source: http://stackoverflow.com/questions/12489/how-do-you-get-a-directory-listing-in-c
char ** listFiles(char *directory, int *numFiles)
{
	char **fileList;      
	DIR *dir;             
	struct dirent *entry; 
	struct stat info;    
	dir = opendir(directory);

	*numFiles = 0;
	fileList = NULL;
	while ((entry = readdir(dir)) != NULL) {
		stat(entry->d_name, &info);
		if (S_ISDIR(info.st_mode)) {
			continue;
		}

		{
			if (fileList == NULL) {
				fileList = malloc(sizeof(char *));
			} else {
				fileList = realloc(fileList, (*numFiles + 1) * sizeof(char *));
			}
			assert(fileList != NULL); 
			fileList[*numFiles] = malloc((strlen(entry->d_name) + 1) * sizeof(char));
			assert(fileList[*numFiles] != NULL);
			strcpy(fileList[*numFiles], entry->d_name);
			(*numFiles)++;
		}
	}

	closedir(dir);
	return fileList;
}

void incomingFile(int socket, void *buffer, int numBytes)
{
	int ret;             
	int receivedBytes;     
	receivedBytes = 0;
	while (receivedBytes < numBytes) {
		ret = recv(socket, buffer + receivedBytes, numBytes - receivedBytes, 0);
		if (ret == -1) {
			perror("recv");
			exit(1);
		}
		else 
		{
			receivedBytes += ret;
		}
	}
}

void checkPacket(int socket, char *tag, char *data)
{
	unsigned short packetLength;    
	unsigned short dataLength;        
	char tmpTag[8 + 1];          
	char tmpData[512 + 1]; 

	incomingFile(socket, &packetLength, sizeof(packetLength));
	packetLength = ntohs(packetLength);

	incomingFile(socket, tmpTag, 8);
	tmpTag[8] = '\0';
	if (tag != NULL) { strcpy(tag, tmpTag); }
	dataLength = packetLength - 8 - sizeof(packetLength);
	incomingFile(socket, tmpData, dataLength);
	tmpData[dataLength] = '\0';
	if (data != NULL) { strcpy(data, tmpData); }
}

int createDataConnection(int controlSocket, int dataSocket, char *commandTag, char *filename)
{
	int ret = 0;    
	char **fileList; 
	int numFiles;   
	int i;		

	fileList = listFiles(".", &numFiles);
	if (strcmp(commandTag, "LIST") == 0) {

		printf("\nsending directory list ...\n");
		for (i = 0; i < numFiles; i++) {
			sendPacket(dataSocket, "FNAME", fileList[i]);
		}
	}

	else if (strcmp(commandTag, "GET") == 0) {
		do {
			char buffer[512 + 1]; 
			int bytesRead;  
			int fileExists; 
			FILE *infile;   

			fileExists = 0;
			for (i = 0; i < numFiles && !fileExists; i++) {
				if (strcmp(filename, fileList[i]) == 0) {
					fileExists = 1;
				}
			}

			infile = fopen(filename, "r");
			if (infile == NULL) {
				sendPacket(controlSocket, "ERROR", "Unable to open file");
				ret = -1;
				break;
			}
			sendPacket(dataSocket, "FILE", filename);
			printf("\nsending file ...\n");
			do {
				bytesRead = fread(buffer, sizeof(char), 512, infile);
				buffer[bytesRead] = '\0';
				sendPacket(dataSocket, "FILE", buffer);
			} while (bytesRead > 0);
			if (ferror(infile)) {
				perror("fread");
				ret = -1;
			}
			fclose(infile);

		} while (0);
	}

	else {
		fprintf(stderr, "command-tag must be \"LIST\" or "
		        "\"GET\"; received \"%s\"\n", commandTag);
		ret = -1;
	}
	sendPacket(dataSocket, "DONE", "");
	sendPacket(controlSocket, "CLOSE", "");

	for (i = 0; i < numFiles; i++) {
		free(fileList[i]);
	}
	free(fileList);
	return ret;
}

void sendFile(int socket, void *buffer, int numBytes)
{
	int foo;         
	int bar;    
	bar = 0;
	while (bar < numBytes) {
		foo = send(socket, buffer + bar, numBytes - bar, 0);

		if (foo == -1) {
			perror("send");
			exit(1);
		}

		else {
			bar += foo;
		}
	}
}

void sendPacket(int socket, char *tag, char *data)
{
	unsigned short packetLength;     
	char tagBuffer[8];           
	packetLength = htons(sizeof(packetLength) + 8 + strlen(data));
	sendFile(socket, &packetLength, sizeof(packetLength));
	memset(tagBuffer, '\0', 8);  
	strcpy(tagBuffer, tag);
	sendFile(socket, tagBuffer, 8);
	sendFile(socket, data, strlen(data));
}

int setConnection(int controlSocket, char *commandTag, int *dataPort, char* filename)
{
	char indata[512 + 1];  
	char intag[8 + 1];           		
	char outdata[512 + 1]; 	
	char outtag[8 + 1];          			

	checkPacket(controlSocket, intag, indata);
	if (strcmp(intag, "DPORT") == 0) { *dataPort = atoi(indata); }
	checkPacket(controlSocket, intag, indata);
	strcpy(commandTag, intag);
	strcpy(filename, indata);

	if (strcmp(intag, "LIST") != 0 && strcmp(intag, "GET") != 0) {
		strcpy(outtag, "ERROR");
		strcpy(outdata, "must be -l or -g");
		sendPacket(controlSocket, outtag, outdata);
		return -1;
	}
	else 
	{
		strcpy(outtag, "OKAY");
		sendPacket(controlSocket, outtag, "");
		return 0;
	}
}