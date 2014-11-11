//#include	"unp.h"
//
//int
//main(int argc, char **argv)
//{
//	int					sockfd;
//	struct sockaddr_in	servaddr, cliaddr;
//
//	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
//
//	bzero(&servaddr, sizeof(servaddr));
//	servaddr.sin_family      = AF_INET;
//	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
//	servaddr.sin_port        = htons(SERV_PORT);
//
//	Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
//
//	dg_echo(sockfd, (SA *) &cliaddr, sizeof(cliaddr));
//}

#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
//#include <unistd.h>
#include <cstdio>
#include <iostream>
using namespace std;

const int SEND_PACKET_SIZE = 2;
const int RECV_PACKET_SIZE = 4096;

unsigned int BKDRHash(char *str, unsigned int len) {
	unsigned int seed = 131;
	unsigned int hash = 0;

	while (*str) {
		hash = hash * seed + (*str++);
	}

	return hash;
}

int main(int argc, char** argv){

	int port=atoi(argv[1]);
	int sockfd;
	if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("opening UDP socket!");
		return -1;
	}

	struct sockaddr_in servaddr, cliaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(port);

	if(bind(sockfd, (sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		perror("binding socket!");
		return -1;
	}

	int count;
	socklen_t clilen = sizeof(cliaddr);
	socklen_t len = clilen;

	char sendbuf[SEND_PACKET_SIZE];
	char recvbuf[RECV_PACKET_SIZE];
	char file_name[255];
	unsigned short serial_no = 0;

	struct stat s;
	int map_size=0;
	long file_size_rev;
	int file ;
	int offset = 0;
	int file_size=0;
	char* filebuf;

	while(1) {
		//?
//		if (filebuf == MAP_FAILED) {
//		      perror("map file failed!");
//		      return -1;
//		    }

		len = clilen;
		count = recvfrom(sockfd, recvbuf, RECV_PACKET_SIZE, 0, (sockaddr *) &cliaddr, &len);
		if (count < 0) {
			if (errno == ETIMEDOUT) {
				printf("[timeout error]");
				//								retransmit = true;
				continue;
			} else {
				perror("error receiving packet");
				return -1;
			}
		}
		unsigned int hash_rev=ntohl(*(unsigned int*)recvbuf);
		unsigned short serial_no_rev=ntohs(*(unsigned short*)(recvbuf + 8));
		unsigned short payload_size_rev = ntohs(*(unsigned short*)(recvbuf + 12));
		unsigned int hash = BKDRHash(recvbuf + 16, 0);

		if(hash!=hash_rev||serial_no_rev!=serial_no){//bad packet, ask client retran
//			*(unsigned short*)(sendbuf) = htons(0xffff);
//			long count = sendto(sockfd, sendbuf, SEND_PACKET_SIZE, 0, (sockaddr*)&cliaddr, len);
//			if (count < 0) {
//				perror("error sending packet");
//				return -1;
//			}
//			continue;
		}
		if(serial_no==0){
			memcpy(file_name, recvbuf + 16, strlen(file_name));
			file_size_rev=ntohl(*(unsigned int*)recvbuf+12);
			long size_high=ntohl(*(unsigned int*)recvbuf+8);
			file_size_rev=file_size_rev+(size_high<<32);
			file = open(file_name, O_WRONLY, O_CREAT);
			fstat(file, &s);
			map_size=payload_size_rev;
		}
		else{
			memcpy(filebuf+offset, recvbuf + 16, payload_size_rev);
			offset+=payload_size_rev;
			file_size+=payload_size_rev;
			if(offset>=map_size||file_size==file_size_rev){
				write(file,filebuf,map_size);
				fstat(file, &s);
				offset=0;
			}
		}

		*(unsigned short*)(sendbuf) = htons(serial_no_rev);// ?

		long count = sendto(sockfd, sendbuf, SEND_PACKET_SIZE, 0, (sockaddr*)&cliaddr, len);
		if (count < 0) {
			perror("error sending packet");
			return -1;
		}
		if(file_size==file_size_rev)
			break;
		serial_no=serial_no_rev+1;
		//get ser num and plus packetsize
		//send back;

	}

	  printf("[completed]");
	  delete[] sendbuf;
	  delete[] recvbuf;
	  close(file);
	  close(sockfd);
	  return 0;
}
