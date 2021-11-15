#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("wrong arguement\n");
		return 0;
	}
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	char buffer[1025] = { 0 };
	char input[1025] = { 0 };
	int pid;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons( strtoul(argv[2],NULL,0) );

	// Convert I Pv4 and IPv6 addresses from text to binary form 
	if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

	pid = fork();
	if (pid == 0){
		//printf("HI");
		while(1){
			if( (valread = read( sock , buffer, 1025)) == 0 ){
				printf("the server has closed.\n");
				break;
			}
			printf("%s",buffer);
			memset(buffer,0,sizeof(buffer[0])*1025);
		}
	}else{
		while(1){
			memset(input,0,sizeof(input[0])*1025);
			fgets(input,1025,stdin);

			if (!strcmp(input,"exit\n")){
				kill(pid, 1);
				return 0;
			}

			send(sock, input, strlen(input), 0);
		}
	}
	
	return 0;
}
