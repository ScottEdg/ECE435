/* 
ECE 435 Final Project: TCP Packet Sniffer: Client RX
Authors: Mike Nichols and Scott Edgerly 

	Sniffer to extract TCP message packets sent over ethernet. 
	Data extracted: 
		Source IP
		Source PORT
		Destination IP
		Destination PORT
		Message length (number of chars in message)
		Message 
*/

#include<pcap.h>
#include<stdio.h>
#include<stdlib.h> 
#include<string.h>  
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<net/ethernet.h>
#include<netinet/tcp.h> 
#include<netinet/ip.h>

// pcap_open_live() constants 
#define ETH0 "eth0"			// Interface name
#define SNAPLEN 20	 		// Size of snapshot length set on handle
#define PROMISC_MODE_ON 1   // Promiscuous mode on 
#define TO_MS 0				// Do no set read out time in ms

// pcap_loop() constants
#define CNT_INF -1			// Process and infinite amount of packets
#define USER_ARG NULL		// pcap_loop user field = NULL

// Miscellaneous constants 
#define ETH_HDR_LEN 14      // Standard ethernet header length = 14 bytes
 
// Message handler: GetMess() 
void GetMess(u_char *, const struct pcap_pkthdr *, const u_char *);

struct sockaddr_in source;	// Struct used to extract source IP address
struct sockaddr_in dest;	// Sturct used to extract destination IP address
  
int main()
{
    pcap_t *handle; 		// Handle of sniffed eth0 device
 	char errbuf[100];
     
    // Open the device for sniffing
    printf("Initializing sniffer.\n");
    handle = pcap_open_live(ETH0, SNAPLEN, PROMISC_MODE_ON, TO_MS, errbuf);
     
    if (handle == NULL) 
    {
        fprintf(stderr, "Unable to sniff on %s : %s\n" , ETH0 , errbuf);
        exit(1);
    }
    printf("Sniffer successfully initialized. \nStarting sniffer loop.\n");

    printf("\n###################################################\n\n");
          
    // Put device in sniff loop. Pass pcap_loop() message handler: GetMess()
    pcap_loop(handle , CNT_INF , GetMess , USER_ARG);    

    return 0;   
}
 
void GetMess(u_char *args,const struct pcap_pkthdr *header,const u_char *buffer)
{
    unsigned int ip_hdr_len;	// IP  header length
    unsigned int tcp_hdr_len;	// TCP header length
    int header_size;			// Total (Eth + IP + TCP) header length
    int message_size = 0; 		// Number of chars transmitted in message

    // IP packet data extraction: See ip.h for iphdr struct member definitions
    struct iphdr *iph = (struct iphdr *)(buffer  +  ETH_HDR_LEN);
    ip_hdr_len = iph->ihl*4;    

    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
     
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;

    // TCP packet data extraction: See tcp.h for tcphdr struct member definition
    struct tcphdr *tcph=(struct tcphdr*)(buffer + ip_hdr_len + ETH_HDR_LEN);
    tcp_hdr_len = tcph->doff*4;
     
    header_size =  ETH_HDR_LEN + ip_hdr_len + tcp_hdr_len; 	// Total header size   

    char * payload = (char *)(buffer + header_size);
    char * message = payload;

    while(*message!=':') message++;

    message_size = strnlen(++message, 256);
    message_size--;  							// Decrement for NULL character

    if (ntohs(tcph->source)==5190)
    { 
	    printf("Source IP             :  %s\n", inet_ntoa(source.sin_addr));
	    printf("Source PORT           :  %u\n", ntohs(tcph->source));
		printf("Destination IP        :  %s\n", inet_ntoa(dest.sin_addr));
		printf("Destination PORT      :  %u\n", ntohs(tcph->dest));
	    printf("Message Length (chars):  %d\n", message_size);
		printf("Message: %s\n", payload);
	    printf("\n###################################################\n\n");
	}
}
