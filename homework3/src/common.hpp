#ifndef COMMON_H
#define COMMON_H

#define BUF_NUM 2000
#define BUF_SIZE 3000
#define _THRESHOLD 16
#define MAX_FRAME_SIZE 40000000
#define DATA_SIZE 4096
#define TIME_OUT 100000

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
} header;

typedef struct{
	header head;
	char data[DATA_SIZE];
} segment;

char msg[50];
inline char *parser(segment *tmp){
	if(tmp -> head.ack && tmp -> head.fin) sprintf(msg, "finack");
	else if(tmp -> head.fin) sprintf(msg, "fin");
	else if(tmp -> head.ack) sprintf(msg, "ack	#%d", tmp -> head.ackNumber);
	else sprintf(msg, "data	#%d", tmp -> head.seqNumber);
	return msg;
}

char buff[BUF_SIZE];
inline int writer(int fd, segment *tmp, int size, struct sockaddr_in *dest, bool resend = 0, int window = 0){
	int p = 0;
	while(p < size){
		int res = sendto(fd, tmp + p, size - p, 0, (struct sockaddr*)dest, sizeof(*dest));
		if(res == -1) res = 0;
		p += res;
	}
	if(resend) printf("resnd	%s", parser(tmp));
	else printf("send	%s", parser(tmp));
	if(window) printf(",	winSize = %d", window);
	printf("\n");
	return 0;
}

socklen_t addrlen = sizeof(struct sockaddr_in);
inline int reader(int fd, segment *tmp, int size, struct sockaddr_in *dest, bool drop = 0){
	int p = 0;
	while(p < size){
		int res = recvfrom(fd, tmp, size, 0, (struct sockaddr*)dest, &addrlen);
		if(res == -1) return -1;
		p += res;
	}
	if(drop == 0 || (tmp -> head.fin) == 1) printf("recv	%s\n", parser(tmp));
	else printf("drop	%s\n", parser(tmp));
	return 0;
}


#endif
