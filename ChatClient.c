/*
** Joshua Hesseltine
** CS372 Project 1
** 28 October 2016
**
** http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpclient.c
** http://www.programminglogic.com/example-of-client-server-program-in-c-using-sockets-and-tcp/
** https://beej.us/guide/bgc/
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h> 
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    
	int n;
	int stayConnected;
	int sockfd;

	char inputBuffer[501];
	char outputBuffer[513];
	char myHandle[11];
	char* portNumber;

    if (argc < 3) 
    {
		printf("Correct: ChatClient [hostname] [port]\n\n");
		exit(0);
    }

	sockfd = connectToServer(argv[1], atoi(argv[2]));

	//readin user input for myHandle
	printf 	("\n Please enter a Client Handle: ");
	fgets 	(myHandle, sizeof(myHandle), stdin);
	printf 	("\n");
	myHandle[strlen(myHandle) - 1] = '\0'; //removing fgets line
	
	portNumber = argv[2];//getting the string of the port number

	//writing the port number to the server after established connection
	n = write(sockfd, portNumber, strlen(portNumber) );
    
    if (n < 0) 
    {
		printf("Error writing to socket\n");
	}

	stayConnected = 1;

	while(stayConnected) 
	{
		printf("%s> ", myHandle);
		bzero(inputBuffer,501);
		fgets(inputBuffer, 500, stdin);
		inputBuffer[strlen(inputBuffer)-1] = '\0';//removing extra line
		
		//end connection client sent quit command
		if (strcmp(inputBuffer, "\\quit") == 0) 
		{
			stayConnected = 0;
			n = write(sockfd, inputBuffer, strlen(inputBuffer));
			if (n < 0) 
			{
				printf("Error writing to socket\n");
			}
		}
		//keep connection alive for more transmissions 
		else 
		{
			bzero(outputBuffer,513);
			strcpy(outputBuffer, myHandle);
			strcat(outputBuffer, "> ");
			strcat(outputBuffer, inputBuffer);
		
			n = write(sockfd, outputBuffer, strlen(outputBuffer));
			if (n < 0) 
			{
				printf("Error writing to socket\n");
			}
			
			//reading from the server
			bzero(outputBuffer, 513);
			n = read(sockfd, outputBuffer, 512);
			if (n < 0) 
			{
				printf("Error reading from socket\n");
			}
				//server sends quit command
				else if(strcmp(outputBuffer, "\\quit") == 0) 
				{//checking if the server wants to quit
					stayConnected = 0;
				}
				//empty server string
				else if (n == 0) 
				{
					stayConnected = 0;
				}
				else 
				{
					printf("%s\n", outputBuffer);
				}
			
		}
	}

	//close connection
	close(sockfd);

    return 0;
}

//creates a connection and returns file descriptor
int connectToServer(char* hostname, int portno) 
{
	int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent* server;
	
	//connecting to server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)  
    {
		printf("Error opening socket");
	}
	
    server = gethostbyname(hostname);
	
    if (server == NULL) 
    {
		printf("Error: host is invalid\n");
		exit(0);
    }
	
    //setting the address
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
    	(char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
	
	//connecting to the server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		printf("Error connecting\n");
		exit(0);
	}
	
	return sockfd;
}
