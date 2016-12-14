#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "pthread.h"

#define PORT "5190" //5190 = AOL Instant Messenger.


#define BUF_SIZE 2000
#define MSG_SIZE 256

int disconnect_client(int fd, fd_set *set, int cur_fd_max);
void broadcast(int sockfd, int nfds, fd_set *set, char *msg);

int main(int argv, char *argc[])
{

	char users[32][19]; //keeps track of users connected to the server
	char msg[MSG_SIZE];
	int fd_max = 3; //need to include stdin, stdout and stderr
	int i;
	int recv_result;
	char *server_ip_addr;
	socklen_t client_len;
	int sockfd; //Socket File Descriptor
	int new_cfd; //Client File Descriptor
	int fd; //used to grab IPv4 Address
//	int tcp = 6; //6 indicates TCP protocol, refer to protocols manpage for more info (man protocols)
	fd_set client_fd_set, read_fd_set;
	struct sockaddr_in server, client_addr;
/*
	 IPv4 AF_INET sockets:

	struct sockaddr_in {
    	short            sin_family;   // e.g. AF_INET, AF_INET6
    	unsigned short   sin_port;     // e.g. htons(3490)
    	struct in_addr   sin_addr;     // see struct in_addr, below
   	 char             sin_zero[8];  // zero this if you want to
	};

	struct in_addr {
    	unsigned long s_addr;          // load with inet_pton()
	};
*/
	struct ifreq ifr;
/*
	       struct ifreq {
               char ifr_name[IFNAMSIZ]; Interface name 
               union {
                   struct sockaddr ifr_addr;
                   struct sockaddr ifr_dstaddr;
                   struct sockaddr ifr_broadaddr;
                   struct sockaddr ifr_netmask;
                   struct sockaddr ifr_hwaddr;
                   short           ifr_flags;
                   int             ifr_ifindex;
                   int             ifr_metric;
                   int             ifr_mtu;
                   struct ifmap    ifr_map;
                   char            ifr_slave[IFNAMSIZ];
                   char            ifr_newname[IFNAMSIZ];
                   char           *ifr_data;
               };
           };
*/

	/* grab the IPv4 address connected to the rpi */

	/* Note: The following code was heavaly reference by
	http://www.geekpage.jp/en/programming/linux-network/get-ipaddr.php */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
 	close(fd);
	server_ip_addr = (char *)inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

 	/* display result */
	printf("Setting up chat server on %s using port %s.\n",server_ip_addr, PORT);
	/* end of referenced code */


	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //refer to socket manpage for more info (man socket)
		perror("Failed to create a socket!\n");
		exit(EXIT_FAILURE);
	}


	/* bind the IP address to the socket file descriptor */
	memset(&server, 0 ,sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(5190);
       // server.sin_addr.s_addr = INADDR_ANY; //connect to any interface avaialble to the machine (I belive this means any IP address)

	if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("Failed to bind socket!");
		exit(EXIT_FAILURE);
	}


	/* Mark the socket as a passive socket and listen for any incoming connection requests */
	if (listen(sockfd, 1) < 0){ //refer to listen manpage for more info (man listen)
					    //SOMAXCONN = 128, refer to file /proc/sys/net/core/somaxconn
		perror("Failure using listen()!");
		exit(EXIT_FAILURE);
	}

	/* Initialize the set of active sockets  */
	FD_ZERO(&client_fd_set);
	FD_SET(sockfd, &client_fd_set);
	fd_max = sockfd + 1;

	while(1){
	/* handle connections here */
		read_fd_set = client_fd_set;  //update the set of file descriptors to read from
		if(select(fd_max, &read_fd_set, NULL, NULL, NULL) < 0){
			perror("Error with select()!");
			exit(EXIT_FAILURE);
		}
		for(i = sockfd; i < fd_max; ++i){ //iterate through all connections
			if(FD_ISSET(i, &read_fd_set)){
				/* If a new connectiion is requested from listen() */
				if(i == sockfd){
					/* Accept any incomming connection requests, */
					client_len = sizeof(client_addr);
					new_cfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_len);
					if (new_cfd < 0){
						perror("Error Accepting socket!");
						//exit(EXIT_FAILURE);
					}
					/* add username to users list, */
					if(recv(new_cfd, &msg, MSG_SIZE,0) < 0){
						perror("Error getting user name from new client");
					}
					memset(users[new_cfd], '\0', 16); //clear if already been used
					strcpy(users[new_cfd], msg);
					strcat(msg, " has connected to the chat room!\n");
					printf("%s", msg);
					printf("New user is %s\n", users[new_cfd]);

					/* and add the new client file descriptor to the set of client file desriptors */
					FD_SET(new_cfd, &client_fd_set);
					if(new_cfd + 1 >= fd_max){
						fd_max = new_cfd + 1;
					}
					/* Broadcast the message to all connections */
					broadcast(sockfd, fd_max, &client_fd_set, msg); 
				}
				/* Otherwise, we are getting new data from an already connected client */
				else{
					recv_result = recv(i, &msg, MSG_SIZE, 0);
					if(recv_result < 0){
						printf("%d", fd_max);
						perror("Error receiving message!");
						exit(EXIT_FAILURE);
					}

					else if(recv_result == 0){
						memset(msg, '\0', 16); //clear if already been used
						strcpy(msg, users[i]);
						strcat(msg, " has disconnected from the chat room.\n");
						printf("%s has disconnected from the chat.\n", users[i]);
						fd_max = disconnect_client(i, &client_fd_set, fd_max);
						broadcast(sockfd, fd_max, &client_fd_set, msg);
					}

					else if(strstr(msg, ": /quit") != NULL){  //search for ": /quit" string in msg, if not found NULL is returned
						memset(msg, '\0', 16); //clear if already been used
						strcpy(msg, users[i]);
						strcat(msg, " has left the chat room.\n");
						printf("%s has left the chat room.\n", users[i]);
						fd_max = disconnect_client(i, &client_fd_set, fd_max);
						broadcast(sockfd, fd_max, &client_fd_set, msg);
					}
					else{
						printf("%s\n", msg);
						broadcast(sockfd, fd_max, &client_fd_set, msg);
					}
				}
			} // End ISSET
		} //end for
	}// end while
	printf("Server shutting down\n");
	close(new_cfd);
	close(sockfd);
	return 0;
}


/* Disconnect the client from the server and return the
   highest numbered file descriptor */
int disconnect_client(int fd, fd_set *set, int cur_fd_max){
	int i;
	int new_fd_max = cur_fd_max; //return the current fd max if we are not closing the current fd max number
	close(fd);
	FD_CLR(fd, set);
	if(fd + 1 == cur_fd_max){ //if the fd we are closing is the current fd max number
		for(i = 0; i < cur_fd_max; i++){
			if(FD_ISSET(fd, set)){
				new_fd_max = i + 1; //we found a new fd max
			}
		}
	}
	return new_fd_max;
}


void broadcast(int sockfd, int nfds, fd_set* set, char *msg){
	int i;
	for(i=sockfd+1; i < nfds; i++){
		if(FD_ISSET(i, set)){
			if(send(i, msg, MSG_SIZE, 0) < 0){
				perror("Error Echoing message to Clients");
			}
		}
	}
}
