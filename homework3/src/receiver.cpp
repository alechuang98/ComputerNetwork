#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <list>
#include <algorithm>
#include "common.hpp"
#include "opencv2/opencv.hpp"
using namespace cv;
using namespace std;

int main(int argc, char *argv[]){
	int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);

	int intg = 1;
	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&intg, sizeof(intg)) < 0){
		perror("SOCKOPT");
	}

	struct sockaddr_in agent;
	memset(&agent, 0, sizeof(agent));
	agent.sin_family = AF_INET;
	agent.sin_port = htons(atoi(argv[2]));
	agent.sin_addr.s_addr = inet_addr(argv[1]);

	struct sockaddr_in receiver;
	memset(&receiver, 0, sizeof(receiver));
	receiver.sin_family = AF_INET;
	receiver.sin_port = htons(atoi(argv[4]));
	receiver.sin_addr.s_addr = inet_addr(argv[3]);
	
	bind(sock_fd,(struct sockaddr *)&receiver,sizeof(receiver));

	segment ack;
	memset(&ack, 0, sizeof(ack));
	ack.head.ack = ack.head.ackNumber = 1;
	
	segment tmp;
	reader(sock_fd, &tmp, sizeof(tmp), &agent);
	writer(sock_fd, &ack, sizeof(ack), &agent);
	
	int h, w;
	sscanf(tmp.data, "%d %d", &h, &w);
	Mat img;
	img = Mat::zeros(h, w, CV_8UC3);
	if(!img.isContinuous()){
		img = img.clone();
	}
	int frame_size = img.total() * img.elemSize(), fptr = 0;
	printf("%d %d %d\n", h, w, frame_size);
	int id = 2;
	while(!ack.head.fin){
		while(fptr < frame_size){
			reader(sock_fd, &tmp, sizeof(tmp), &agent);
			int copy_size = tmp.head.length;
			if(tmp.head.seqNumber > id) ack.head.ackNumber = id - 1;
			else {
				ack.head.ackNumber = id ++;
				memcpy(img.data + fptr, tmp.data, copy_size);
				fptr += copy_size;
			}
			writer(sock_fd, &ack, sizeof(ack), &agent);
		}
		fptr = 0;
		reader(sock_fd, &tmp, sizeof(tmp), &agent, 1);
		puts("flush");
		imshow("Video", img);
		char c = (char)waitKey(33.3333);
		if(c == 27) break;
		if(tmp.head.fin){
			ack.head.fin = 1;
			writer(sock_fd, &ack, sizeof(ack), &agent);
		}
	}
	destroyAllWindows();
}
