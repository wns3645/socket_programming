#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"
#include <stddef.h>

// Client-side

int
main(int argc, char const * argv[])
{
	int client_socket;
	struct sockaddr_in serv_addr;
	
	struct message *sending_message;
	struct message *received_message;
	uint32_t ip_addr;
	uint16_t port;
	uint16_t op;
	char keyword[5];

	// command-line argument parsing
	for(int i=1; i<argc; i++)
	{
		if(!strncmp(argv[i], "-h", 3))
		{
			i++;
			ip_addr = inet_addr(argv[i]);
		}
		else if(!strncmp(argv[i], "-p", 3))
		{
			i++;
			port = htons(atoi(argv[i]));
		}
		else if(!strncmp(argv[i], "-o", 3))
		{
			i++;
			op = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-k", 3))
		{
			i++;
			strncpy(keyword, argv[i], 5);
		}
	}

	// make a client socket 
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(client_socket == -1)
	{
		printf("socket failed\n");
		exit(1);
	}

	// set up a server IP address and port
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = port;
	serv_addr.sin_addr.s_addr = ip_addr;

	// request a connection to the server
	if( connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1 )
	{
		printf("connection failed\n");
		exit(1);
	}

	sending_message = (struct message*) malloc(sizeof(struct message));
	received_message= (struct message*) malloc(sizeof(struct message));
	memset(sending_message, 0, sizeof(struct message));
	memset(received_message, 0, sizeof(struct message));

	// assume that data is always less than 10MB
	int read_size;
	int receive_size;
	while((read_size = fread(sending_message->data, 1, MAX_SIZE-16, stdin)) > 0)
	{	
		sending_message->op = htons(op);
		strncpy(sending_message->keyword, keyword, 4);
		sending_message->length = htonll((uint64_t)16 + read_size);
		sending_message->chksum = ip_checksum((char *)sending_message, 16 + read_size, 0);

		// send message
		write(client_socket, sending_message, 16+read_size);

		receive_size = 0;
		while(receive_size < read_size + 16)
		{
			receive_size += read(client_socket, (char *) received_message + receive_size, MAX_SIZE-receive_size);
		}

		if(ip_checksum((char *) received_message, (int) ntohll(received_message->length), 1) != 0xFFFF)
		{ 
			printf("checksum failed\n");
			exit(1);
		}

		fwrite(received_message->data, 1, receive_size-16, stdout);
		
		memset(received_message, 0, sizeof(struct message));
		memset(sending_message, 0, sizeof(struct message));
	}

	free(sending_message);
	free(received_message);

	close(client_socket);

	return 0;
}
