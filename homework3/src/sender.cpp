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
#include <iostream>
using namespace cv;
using namespace std;

char frame[MAX_FRAME_SIZE];
int fptr, start, frame_size;
int getFrame(char *data, VideoCapture &cap, Mat &img){
	if(!start){
		start = 1;
		int h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
		int w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
		img = Mat::zeros(h, w, CV_8UC3);
		if(!img.isContinuous()){
			img = img.clone();
		}
		frame_size = img.total() * img.elemSize();
		cap >> img;
		sprintf(data, "%d %d\n", img.rows, img.cols);
		return strlen(data);
	} else {
		int dptr = 0;
		while(dptr < DATA_SIZE){
			if(fptr == frame_size){
				cap >> img;
				fptr = 0;
				break;
			}
			if(img.empty()) return 0;
			int copy_size = min(frame_size - fptr, DATA_SIZE - dptr);
			memcpy(data + dptr, img.data + fptr, copy_size);
			fptr += copy_size;
			dptr += copy_size;
		}
		return dptr;
	}
}

// IP, port, source file
int main(int argc, char *argv[]){
	int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = TIME_OUT;
	if(setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0){
		perror("SOCKOPT RCV");
	}
	int intg = 1;
	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&intg, sizeof(intg)) < 0){
		perror("SOCKOPT");
	}

	struct sockaddr_in agent;
	memset(&agent, 0, sizeof(agent));
	agent.sin_family = AF_INET;
	agent.sin_port = htons((unsigned short)atoi(argv[2]));
	agent.sin_addr.s_addr = inet_addr(argv[1]);

	struct sockaddr_in sender;
	memset(&sender, 0, sizeof(agent));
	sender.sin_family = AF_INET;
	sender.sin_port = htons((unsigned short)atoi(argv[4]));
	sender.sin_addr.s_addr = inet_addr(argv[3]);

	bind(sock_fd,(struct sockaddr *)&sender,sizeof(sender));

	VideoCapture cap(argv[5]);
	Mat img;
	deque<segment> que;
	deque<segment>::iterator it;
	it = que.end();
	int window = 1, threshold = _THRESHOLD, id = 1, end = 0, pre = 0, ocnt = 0;
	while(!end or !que.empty()){
		segment tmp;
		memset(&tmp, 0, sizeof(tmp));
		while(pre < window and !(it == que.end() and end)){
			if(it == que.end()){
				tmp.head.seqNumber = id ++;
				tmp.head.length = getFrame(tmp.data, cap, img);
				if(tmp.head.length == 0){
					end = 1;
					break;
				}
				que.push_back(tmp);
				writer(sock_fd, &tmp, sizeof(tmp), &agent, 0, window);
				it = que.end();
			} else {
				writer(sock_fd, &(*it), sizeof(*it), &agent, 1, window);
				++ it;
			}
			pre ++;
		}
		pre = window;
		if(!que.empty()){
			int res = reader(sock_fd, &tmp, sizeof(tmp), &agent);
			if(res == -1){
				threshold = max(window >> 1, 1);
				ocnt = 0;
				window = 1;
				pre = 0;
				it = que.begin();
				printf("time	out,		threshold = %d\n", threshold);
			} else {
				while(!que.empty() and que.front().head.seqNumber <= tmp.head.ackNumber){
					pre --;
					que.pop_front();
					window += window < threshold ? 1 : (++ ocnt == window ? 1 + (ocnt = 0) : 0);
				}
			}
		}
	}
	segment tmp;
	memset(&tmp, 0, sizeof(tmp));
	tmp.head.fin = 1;
	writer(sock_fd, &tmp, sizeof(tmp), &agent);
	reader(sock_fd, &tmp, sizeof(tmp), &agent);
	cap.release();
}
		
