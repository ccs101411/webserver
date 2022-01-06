#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <pcap/pcap.h>
#include <time.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/ipv6.h>
void print_packet_info(
		u_char *args,
		const struct pcap_pkthdr *header,
		const u_char *packet
		);

int main(int argc, char *argv[])  
{  
    char *path;
	const u_char *packet;
	struct pcap_pkthdr header;
	if(argc < 2)
	{
		perror("No File Path");
		exit(0);
	}
	else
	{
		path = argv[1];
	}
	char error_buffer[PCAP_ERRBUF_SIZE];
	pcap_t *handle = pcap_open_offline_with_tstamp_precision\
					 (path, PCAP_TSTAMP_PRECISION_MICRO, error_buffer);
	if(handle == NULL)
	{
		perror("open pcap fail");
		exit(0);
	}
	pcap_loop(handle, 0, print_packet_info, NULL);
	return 0;
}


void print_packet_info(
		u_char *args,
		const struct pcap_pkthdr *header,
		const u_char *packet
		)
	{
    ////time stamp
	printf("timesec: %08lX\n",header->ts.tv_sec);
	printf("microsec: %08lX\n",header->ts.tv_usec);
	printf("timestamp: %s", ctime((const time_t*)&header->ts.tv_sec));	
	////
	////show MAC and type
	struct ether_header *eth = (struct ether_header*) packet;
	u_int16_t type = ntohs(eth->ether_type);
	printf("Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",\
	eth->ether_shost[0], eth->ether_shost[1], eth->ether_shost[2],\
	eth->ether_shost[3], eth->ether_shost[4], eth->ether_shost[5]);
	printf("Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",\
	eth->ether_dhost[0], eth->ether_dhost[1], eth->ether_dhost[2],\
	eth->ether_dhost[3], eth->ether_dhost[4], eth->ether_dhost[5]);

	printf("Type: %04X\n",type);
	////

	////locate IP or TCP/UDP
	if((type == ETHERTYPE_IP) || (type == ETHERTYPE_IPV6))
	{
		struct iphdr *iphd = (struct iphdr*)\
			(packet + sizeof(struct ethhdr));
		printf("IP verison: %d\n", iphd->version);
		if(iphd->version == 6){
			struct ipv6hdr *ipv6hd = (struct ipv6hdr*)\
					(packet + sizeof(struct ethhdr));	
			printf("Source IP: ");
			for(int i=0;i<16;i++)
				if(i % 2 == 1 && i != 15)
					printf("%02X:",ipv6hd->saddr.s6_addr[i]);
				else
					printf("%02X",ipv6hd->saddr.s6_addr[i]);
			printf("\nDest IP: ");
			for(int i=0;i<16;i++)
				if(i % 2 == 1 && i != 15)
					printf("%02X:",ipv6hd->daddr.s6_addr[i]);
				else
					printf("%02X",ipv6hd->daddr.s6_addr[i]);
			printf("\n");
		}
		else
		{
			printf("Source IP: %s\n", inet_ntoa(*(struct in_addr*)\
						&iphd->saddr));
			printf("Dest IP: %s\n", inet_ntoa(*(struct in_addr*)\
						&iphd->daddr));
		}
		if(iphd->protocol == 6)
		{
			struct tcphdr *tcphd = (struct tcphdr*)\
			(packet + sizeof(struct ethhdr) + sizeof(struct iphdr));
			printf("TCP ports:\n");
			printf("\tSource: %d\n", ntohs(tcphd->source));
			printf("\tDest: %d\n", ntohs(tcphd->dest));
		}
		else if(iphd->protocol == 17)
		{
			struct udphdr *udphd = (struct udphdr*)\
			(packet + sizeof(struct ethhdr) + sizeof(struct iphdr));
			printf("UDP ports:\n");
			printf("\tSource: %d\n", ntohs(udphd->source));
			printf("\tDest: %d\n", ntohs(udphd->dest));
		}
	}
	else
	{
		printf("Not IP packet\n");
	}
	////end of function print_packet_info
	printf("////////////////////////\n");
}
