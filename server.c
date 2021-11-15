#include <stdio.h>  
#include <string.h>
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h>

struct user{
    char addr[16];
    int port;
    char name[13];
}users[20];

int main(int argc , char *argv[])   {
	if (argc != 2){
		fprintf(stderr, "Usage: ./server <PORT>\n");
		return 0;
	}
	int opt = 1;   
    int master_socket , addrlen , new_socket , client_socket[20] ,  
          max_clients = 20 , activity, i , valread , sd;   
    int max_sd;   
    struct sockaddr_in address;  
    char buffer[1025] = { 0 };  //data buffer of 1K
    char message[1025] = { 0 };
         
    //set of socket descriptors  
    fd_set readfds;   
         
    //initialise all client_socket[] to 0 so not checked  
    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;   
    }   
         
    //create a master socket
    if( (master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    //set master socket to allow multiple connections
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )   {
        perror("Setsockopt");   
        exit(EXIT_FAILURE);   
    }   
     
    //type of socket created  
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( strtoul(argv[1],NULL,0) );   
         
    //bind the socket to localhost port  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   {
        perror("Bind failed");   
        exit(EXIT_FAILURE);   
    }   
         
    //try to specify maximum pending connections for the master socket  
    if (listen(master_socket, 10) < 0){
        perror("Listen");   
        exit(EXIT_FAILURE);   
    }   
         
    //accept the incoming connection  
    addrlen = sizeof(address);    
         
    while(1){
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(master_socket, &readfds);   
        max_sd = master_socket;
             
        //add child sockets to set  
        for ( i = 0 ; i < max_clients ; i++) {
            //socket descriptor
            sd = client_socket[i];
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR)){   
            printf("select error");   
        }   
             
        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(master_socket, &readfds)){
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                perror("accept");
                exit(EXIT_FAILURE);   
            }
             
            //inform user of socket number - used in send and receive commands  
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   

            memset(message,0,sizeof(message[0])*1025);
            //a message
            sprintf(message,"%s %s:%d\n", "[Server] Hello, anonymous! From:", inet_ntoa(address.sin_addr) , ntohs (address.sin_port));
            
            //puts(message);
            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message)){
                perror("send");   
            }   
                 
            puts("Welcome message sent successfully");   

            //add new socket to array of sockets  
            for (i = 0; i < max_clients; i++) {
                //if position is empty
                if( client_socket[i] == 0 ){
                    client_socket[i] = new_socket;   
                    printf("Adding to list of sockets as %d\n" , i);

                    memset( users[i].addr,0,16*sizeof(users[i].addr[0]) );
                    memset( users[i].name,0,13*sizeof(users[i].name[0]) ); 

                    sprintf(users[i].addr,"%s", inet_ntoa(address.sin_addr));
                    sprintf(users[i].name,"anonymous");
                    users[i].port = ntohs (address.sin_port);
                    
                    break;   
                }   
            }

            //send sockets "someone is coming"
            for (int i = 0;i < max_clients;i++){
                if (client_socket[i]){
                    memset(message,0,1025*sizeof(message[0]));
                    if (users[i].port == ntohs(address.sin_port) && !strcmp(inet_ntoa(address.sin_addr), users[i].addr)){
                    } else {
                        sprintf(message,"[Server] Someone is coming!\n");
                        send(client_socket[i],message,strlen(message),0);
                    }
                }
            }

        }   

        //else its some IO operation on some other socket 
        for (i = 0; i < max_clients; i++){
            sd = client_socket[i];
                 
            if (FD_ISSET( sd , &readfds)){
                //Check if it was for closing , and also read the  
                //incoming message  
                if ((valread = read( sd , buffer, 1025)) == 0){   
                    //Somebody disconnected , get his details and print  
                    getpeername(sd , (struct sockaddr*)&address ,
                        (socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                         
                    //Close the socket and mark as 0 in list for reuse  
                    close( sd );   
                    client_socket[i] = 0;

                    for (int j = 0;j < max_clients;j++){
                        if (client_socket[j]){
                            memset(message,0,1025*sizeof(message[0]));
                            if (users[j].port == ntohs(address.sin_port) && !strcmp(inet_ntoa(address.sin_addr), users[j].addr) ){

                            }else{
                                memset(message,0,1025*sizeof(message[0]));
                                sprintf(message,"[Server] %s is offline.\n",users[i].name);
                                send(client_socket[j],message,strlen(message),0);
                            }
                        }
                    }

                }else{
                    //Echo back the message that came in     
                    //set the string terminating NULL byte on the end  
                    //of the data read  
                    printf("[%s:%d] %s",inet_ntoa(address.sin_addr),ntohs(address.sin_port),buffer);

                    char str[5] = { 0 };
                    for (int j = 0;j < 4;j++)
                        str[j] = buffer[j];
                    
                    if (!strncmp(str,"who",3)){
                        puts(str);

                        for (int z = 0; z < max_clients; z++){
                            if (client_socket[z]){
                                memset(message, 0, sizeof(message[0])*1025);
                                sprintf(message,"[Server] %s %s:%d",users[z].name,users[z].addr,users[z].port);
                                if ( z == i ){
                                    sprintf(message,"%s ->me",message);
                                }
                                sprintf(message,"%s\n",message);
                                send(sd , message , strlen(message) , 0 );
                            }
                        }
                    }else if( !strcmp(str,"name") ){
                        puts(str);
                        memset(message,0,1025*sizeof(message[0]));
                        char n[15] = { 0 };

                        int check = 0;
                        if (strlen(buffer) > 6){
                            for (int j = 5;buffer[j] != '\n';j++){
                                n[j-5] = buffer[j];
                                if (n[j-5]<'a' || n[j-5]>'z')
                                    check = 1;
                            }
                        }

                        if (check || (strlen(n)<2) || (strlen(n)>12) ){
                            sprintf(message,"[Server] ERROR: Username can only consists of 2~12 English letters.\n");
                        }else{
                            if (!strcmp(n,"anonymous")){
                                sprintf(message,"[Server] ERROR: Username cannot be anonymous.\n");
                            }else{
                                for (int z = 0;z < max_clients;z++){
                                    if (z != i){
                                        if (!strcmp(users[z].name,n)){
                                            check = 1;
                                        }
                                    }
                                }
                                if (check){
                                    sprintf(message,"[Server] ERROR: %s has been used by others.\n",n);
                                }else{
                                    //success
                                    send(sd,message,strlen(message),0);
                                    memset(message,0,1025*sizeof(message[0]));

                                    //broadcast
                                    sprintf(message,"[Server] %s is now known as %s.\n",users[i].name,n);
                                    for (int z = 0;z < max_clients;z++){
                                        if (i != z){
                                            send(client_socket[z],message,strlen(message),0);
                                        }
                                    }
                                    memset(users[i].name,0,13*sizeof(users[i].name[0]));
                                    sprintf(users[i].name,"%s",n);
                                    sprintf(message,"[Server] You're now known as %s.\n",users[i].name);
                                }
                                
                            }
                        }

                        //send to sender
                        send(sd,message,strlen(message),0);

                    }else if (!strcmp(str,"tell")){
                        puts(str);
                        char msg[1025] = { 0 };
                        memset(message,0,1025*sizeof(message[0]));
                        if (strlen(buffer) <= 6)
                            continue;
                        int j = 4;
                        j++;
                        char target[13] = { 0 };
                        for (;buffer[j] != ' ';j++){
                            target[j-5] = buffer[j];
                        }
                        j++;
                        for (int z = 0;buffer[j] != '\n';j++,z++){
                            msg[z] = buffer[j];
                        }
                        int target_idx = -1;
                        if ( !strcmp(users[i].name,"anonymous") ){
                            if (!strcmp("anonymous", target) )
                                sprintf(message,"[Server] ERROR: You are anonymous.\n[Server] ERROR: The client to which you sent is anonymous.\n");
                            else
                                sprintf(message,"[Server] ERROR: You are anonymous.\n");
                        }else{
                            if ( !strcmp("anonymous",target) ){
                                sprintf(message,"[Server] ERROR: The client to which you sent is anonymous.\n");
                            }else{
                                for (int z = 0;z < max_clients;z++){
                                    if ( !strcmp(users[z].name,target) ){
                                        target_idx = z;
                                    }
                                }
                                if (target_idx >= 0){
                                    sprintf(message,"[Server] SUCCESS: Your message has been sent.\n");
                                }else{
                                    sprintf(message,"[Server] ERROR: The receiver doesn't exist.\n");  
                                }
                            }
                        }

                        send(sd , message , strlen(message) , 0 );
                        memset(message,0,1025*sizeof(message[0]));
                        sprintf(message,"[Server] %s tell you %s\n",users[i].name,msg);
                        send(client_socket[target_idx],message,strlen(message),0);
                            
                    }else if (!strcmp(str,"yell")){
                        puts(str);
                        char m[1024] = { 0 };
                        for (int j = 4;buffer[j] != '\n';j++){
                            m[j-4] = buffer[j];
                        }
                        
                        for (int z = 0;z < max_clients;z++){
                            if (client_socket[z]){
                                memset(message,0,sizeof(message[0])*1025);
                                if (!users[i].name[0])
                                    sprintf(message,"[Server] %s","anonymous");
                                else
                                    sprintf(message,"[Server] %s",users[i].name);
                                
                                sprintf(message, "%s yell%s\n",message,m);
                                send(client_socket[z] , message , strlen(message) , 0 );
                            }
                        }

                    }else{
                        memset(message,0,sizeof(message[0])*1025);
                        sprintf(message,"[Server] ERROR: Error command.\n");
                        //puts(message);
                        send(sd , message , strlen(message) , 0 );
                    }
                }
                memset(buffer,0,sizeof(buffer[0])*1025);
            }   
        }   
    }   
         
    return 0;   
}   