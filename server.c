#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "protocol.h"

// Server-side

void cipher(char *text, char *answer, int text_size, char *keyword, uint16_t op);

int 
main(int argc, char const * argv[])
{
	int server_socket;
	int client_socket;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;

	struct message *received_message;
	struct message *sending_message;
	int length_to_receive;
	uint16_t port;
	
	pid_t pid;

	// command-line argument parsing
	for(int i=1; i<argc; i++)
	{
		if(!strncmp(argv[i], "-p", 3))
		{
			i++;
			port = htons(atoi(argv[i]));
		}
	}

	// make a server socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket == -1)
	{
		printf("socket failed\n");
		exit(1);
	}

	// set up a server IP addr and port
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = port;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


	// socket recreating error handling
	int option = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	// bind socket to kernel
	if(bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		printf("bind failed\n");
		exit(1);
	}

	if(listen(server_socket, 5) == -1)
	{
		printf("listen failed\n");
		exit(1);
	}

	while(1)
	{
		// waiting for accepting client
		socklen_t client_addr_size = sizeof(client_addr);
		client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);

		if(client_socket == -1)
		{
			printf("accept failed\n");
			exit(1);
		}
		else
			printf("client accept, fd = %d\n", client_socket);

		if((pid=fork()) == -1)
		{
			close(client_socket);
			printf("fork failed\n");
			continue;
		}
		else if(pid==0)
		{
			close(server_socket);

			received_message = (struct message*) malloc(sizeof(struct message));
			sending_message = (struct message*) malloc(sizeof(struct message));
			memset(received_message, 0, sizeof(struct message));
			memset(sending_message, 0, sizeof(struct message));
			
			// receive content of message
			while(read(client_socket, received_message, sizeof(struct message))>0)
			{
				if(ip_checksum((char *) received_message, (int) ntohll(received_message->length), 1) != 0xFFFF)
				{
					printf("checksum failed\n");
					exit(1);
				}

				length_to_receive = (int) (ntohll(received_message->length) - 16);

				// encryption or decryption
				cipher(received_message->data, sending_message->data, length_to_receive, received_message->keyword, ntohs(received_message->op));
				sending_message->op = received_message->op;
				strncpy(sending_message->keyword, received_message->keyword, 4);
				sending_message->length = received_message->length;
				sending_message->chksum = ip_checksum((char *)sending_message, 16 + length_to_receive, 0);

				// reply to client
				write(client_socket, sending_message, 16+length_to_receive);
				memset(received_message, 0, sizeof(struct message));
				memset(sending_message, 0, sizeof(struct message));
			}
			
			free(received_message);
			free(sending_message);

			close(client_socket);
			break;
		}
		else
			close(client_socket);
	}

	close(server_socket);
	return 0;

}

void cipher(char *text, char *answer, int text_size, char *keyword, uint16_t op)
{
	int i;
	int k=0;

	// encryption
	if(op == 0)
	{
		for(i=0; i<text_size; i++)
		{
			if(tolower(text[i]) >= 97 && tolower(text[i]) <=122){
				if(tolower(text[i])+keyword[k%4]-97 > 122)
					memset(answer+i, tolower(text[i])+keyword[k%4]-97-26, 1);
				else
					memset(answer+i, tolower(text[i])+keyword[k%4]-97, 1);
				k++;
			}
			else
				memset(answer+i, tolower(text[i]), 1);
		}
	}
	// decryption
	else if(op == 1)
	{
		for(i=0; i<text_size; i++)
		{
			if(tolower(text[i]) >= 97 && tolower(text[i]) <=122){
				if(tolower(text[i])-keyword[k%4]+97 < 97)
					memset(answer+i, tolower(text[i])-keyword[k%4]+97+26, 1);
				else
					memset(answer+i, tolower(text[i])-keyword[k%4]+97, 1);
				k++;
			}
			else
				memset(answer+i, tolower(text[i]), 1);
		}
	}

}



