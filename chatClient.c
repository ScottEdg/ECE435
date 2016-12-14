#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#define MSG_SIZE 256

void * acquire(void * fd);

int main(int argv, char * argc[]){

	int sockfd; //Socket File Descriptor
//	int tcp = 6; //6 indicates TCP protocol, refer to protocols manpage for more info (man protocols)
	int spamCount = 0;
	struct addrinfo hints, *server_info;
	char address[15] = "192.168.1.124";
	char msg[MSG_SIZE]; //msg = nameplate + text. example; [user_name]: text.
	char text[MSG_SIZE]; //the text the user wants to send
	char last_text[MSG_SIZE]; //uesed to check for spam
	char user_name[17]; //nameplate = [user_name]: 
	int nameplate_len = 0;
	char nameplate[21];
	int i;
	pthread_t athread; //usesd for "acquireing" messages.


	/* Get IP address of server from user */
	puts("Enter IP address of chat room server.");
	if (fgets(address, 15, stdin) == NULL){
		perror("error using fgets");
	}

	/* Get user name */
	puts("Please enter a user name (up to 16 characters long)");
	if (fgets(user_name, 17, stdin) == NULL){
		perror("error using fgets");
	}

	/* Remove the newline character returned from fgets */
	user_name[strcspn(user_name, "\n")] = '\0';

	/* Create the nameplate */
	strcpy(nameplate, "[");
	strcat(nameplate, user_name);
	strcat(nameplate, "]: ");

	/* find out how long the nameplate is. */
	nameplate_len = strnlen(nameplate, 21);

        /* create an endpoint for the client connection */
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //refer to socket manpage for more info (man socket)
                perror("Failed to create a socket!");
                exit(EXIT_FAILURE);
        }
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	//hints.ai_protocol = 0;


	/* Get info about the server. */
	if (getaddrinfo(address, "5190", &hints, &server_info)!= 0){
		perror("Failed to get specified IP address info!");
		exit(EXIT_FAILURE);
	}


	/* Connect to the server */
	if(connect(sockfd, server_info->ai_addr, server_info->ai_addrlen) < 0){
		perror("Failed to connect to server");
		exit(EXIT_FAILURE);
	}
	puts("\n\nWelcome to the chat room!  Please, no spamming!\n");
	/* Send user's nameplate to the server. */
	memset(msg, '\0', 16); //clear the msg.
	strncpy(msg, nameplate, nameplate_len - 2); //we don't want to send ": ".
	send(sockfd, &msg, nameplate_len, 0);


	/* Prepend the nameplate to all the following outgoing messages. */
	strcpy(msg, nameplate);


	/* Create a thread the "acquire" all incomming messages from server. */
	pthread_create(&athread, NULL, acquire, (void *) sockfd);


	while(1){
		/* Prepare msg for string concatenation. */
		memset(&msg[nameplate_len], '\0', 1);
		fgets(text, MSG_SIZE, stdin); //get the mesage from user input.
		text[strcspn(text, "\n")] = '\0'; //remove newline genereated by fgets
		strcat(msg, text); //msg is now ready to be sent.
		if(strcmp(text, "/quit") == 0){  //typical exit condtion
			send(sockfd, &msg, MSG_SIZE, 0);
			pthread_cancel(athread);
			break;
		}
		else if(strcmp(text, "") == 0 || spamCount == 10){ //first condition prevents user from sending blank strings to the server
			if(spamCount == 10){
				for(i = 0; i < 250000; i++){
					puts("GIT OUTTA HERE YA FILTHY SPAMMER!");
				}
				break;
			}
			spamCount++;
		}
		else{
			if(strcmp(text, last_text) == 0){
				spamCount++;
			}
			else{
				spamCount = 0;
			}
			send(sockfd, &msg, MSG_SIZE, 0);
			strcpy(last_text, text);
		}
	}
	freeaddrinfo(server_info);
	close(sockfd);
	return 0;
}


void * acquire(void * fd){
	int recv_state;
	int rfd = (int) fd;
	char rmsg[MSG_SIZE];
	while(1){
		recv_state = recv(rfd, &rmsg, MSG_SIZE, 0);
		if (recv_state < 0){
			perror("error receiving messge from server");
			break;
		}
		else if(recv_state == 0){
			break;
		}
		else{
			puts(rmsg);
		}
	}
	pthread_exit(NULL);
}
