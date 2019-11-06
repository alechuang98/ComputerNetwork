#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include "opencv2/opencv.hpp"
#include "constant.hpp"
using namespace std;
using namespace cv;
#define ERR_EXIT(a) { perror(a); exit(1); }

void reader(int fd, char *buf, int size){
	int p = 0;
	while(p < size){
		int res = recv(fd, buf + p, size - sizeof(char) * p, 0);
		p += res;
	}
	printf("in: %d\n", p);
}
void writer(int fd, char *buf, int size){
	int p = 0;
	printf("out %d\n", size);
	while(p < size){
		int res = send(fd, buf + p, size - sizeof(char) * p, 0);
		p += res;
	}
}
int getFileLen(FILE *fp){
	fseek(fp, 0L, SEEK_END);
	int res = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return res;
}

class node{
public:
	int state, size;
	Mat img;
	VideoCapture cap;
	FILE *fp;
	node(){
		state = size = 0;
	}
};

int initServer(unsigned short port){
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(port);

	int tmp = 1;
	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0){
		ERR_EXIT("setsockopt");
	}
	if(bind(sock_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0){
		ERR_EXIT("bind");
	}
	if(listen(sock_fd, MAX_BACKLOG) < 0){
		ERR_EXIT("listen");
	}
	return sock_fd;
}
void setFdSet(fd_set *r_fd_set, fd_set *w_fd_set, node *client){
	FD_ZERO(r_fd_set);
	FD_ZERO(w_fd_set);
	for(int i = 3; i < MAX_FD; i ++){
		if(!client[i].state) continue;
		if(client[i].state <= 2 || client[i].state == STREAM) FD_SET(i, r_fd_set);
		if(client[i].state >= 3) FD_SET(i, w_fd_set);
	}
}

char buf[MAX_BUF], file_name[HEADER_SIZE];
void cmdHandler(node &cli, int fd){
	reader(fd, buf, HEADER_SIZE);
	printf("get cmd from %d. command is %s\n", fd, buf);
	string cmd = string(buf);
	if(cmd == "ls"){
		cli.state = LS;
	} else if(cmd.substr(0, 4) == "get "){
		strcpy(file_name, cmd.substr(4).c_str());
		printf("file_name: %s\n", file_name);
		if(access(file_name, F_OK) == -1){
			puts("NO exist");
			sprintf(buf, "The %s doesn't exist.", file_name);
			writer(fd, buf, HEADER_SIZE);
			return;
		}
		cli.state = DOWNLOAD;
		cli.fp = fopen(file_name, "rb");
		cli.size = getFileLen(cli.fp);
		sprintf(buf, "%d", cli.size);
		writer(fd, buf, HEADER_SIZE);
	} else if(cmd.substr(0, 4) == "put "){
		strcpy(file_name, cmd.substr(4).c_str());
		cli.state = UPLOAD;
		cli.fp = fopen(file_name, "wb");
		cli.size = -1;
	} else if(cmd.substr(0, 5) == "play "){
		strcpy(file_name, cmd.substr(5).c_str());
		if(access(file_name, F_OK) == -1){
			sprintf(buf, "The %s doesn't exist.", file_name);
			writer(fd, buf, HEADER_SIZE);
			return;
		}
		if(cmd.substr(cmd.length() - 4) != ".mpg"){
			sprintf(buf, "The %s is not a mpg file.", file_name);
			writer(fd, buf, HEADER_SIZE);
			return;
		}
		cli.state = STREAM;
		cli.cap = VideoCapture(file_name);
		int h = cli.cap.get(CV_CAP_PROP_FRAME_HEIGHT);
		int w = cli.cap.get(CV_CAP_PROP_FRAME_WIDTH);
		cli.img = Mat::zeros(h, w, CV_8UC3);
		if(!cli.img.isContinuous()){
			cli.img = cli.img.clone();
		}
		cli.size = cli.img.total() * cli.img.elemSize();
		sprintf(buf, "%d %d\n", cli.img.rows, cli.img.cols);
		writer(fd, buf, HEADER_SIZE);
	}
}
void uploadHandler(node &cli, int fd){
	if(cli.size == -1){
		reader(fd, buf, HEADER_SIZE);
		cli.size = atoi(buf);
	} else {
		reader(fd, buf, BUF_SIZE);
		fwrite(buf, sizeof(char), min(BUF_SIZE, cli.size), cli.fp);
		cli.size -= min(BUF_SIZE, cli.size);
		if(cli.size == 0){
			printf("finish upload from %d\n", fd);
			cli.state = CMD;
			fclose(cli.fp);
			return;
		}
	}
}
void downloadHandler(node &cli, int fd){
	fread(buf, sizeof(char), min(BUF_SIZE, cli.size), cli.fp);
	writer(fd, buf, BUF_SIZE);
	cli.size -= min(BUF_SIZE, cli.size);
	if(cli.size == 0){
		cli.state = CMD;
		printf("finish download %d\n", fd);
		fclose(cli.fp);
		return;
	}
}
void streamBrocaster(node &cli, int fd){
	if(cli.size == -1) return;
	cli.cap >> cli.img;
	if(cli.img.total() * cli.img.elemSize() == 0){
		strcpy(buf, END_TAG);
		writer(fd, buf, cli.size);
		cli.size = -1;
	} else {
		memcpy(buf, (char *)cli.img.data, cli.size);
		writer(fd, buf, cli.size);
	}
}
void streamHalter(node &cli, int fd){
	reader(fd, buf, HEADER_SIZE);
	if(cli.size != -1){
		strcpy(buf, END_TAG);
		writer(fd, buf, cli.size);
	}
	cli.state = CMD;
	cli.cap.release();
}
void lsHandler(node &cli, int fd){
	if(getcwd(file_name, HEADER_SIZE) == NULL){
		ERR_EXIT("getcwd");
	} else {
		string res = "";
		DIR *d = opendir(file_name);
		for(struct dirent *de = NULL; (de = readdir(d)) != NULL; ){
			res.append(de -> d_name);
			res.append(" ");
		}
		strcpy(buf, res.c_str());
		writer(fd, buf, BUF_SIZE);
		cli.state = CMD;
	}
}
int main(int argc, char *argv[]){
	int sock_fd = initServer(atoi(argv[1]));
	node client[MAX_FD];
	client[sock_fd].state = CMD;
	struct sockaddr_in cli_addr;
	fd_set r_fd_set, w_fd_set;
	printf("sock fd: %d\n", sock_fd);
	while(1){
		setFdSet(&r_fd_set, &w_fd_set, client);
		for(int i = 3; i < MAX_FD; i ++) if(FD_ISSET(i, &r_fd_set)) printf("%d is set to read.\n", i);
		for(int i = 3; i < MAX_FD; i ++) if(FD_ISSET(i, &w_fd_set)) printf("%d is set to write.\n", i);
		puts("selecting...");
		select(MAX_FD, &r_fd_set, &w_fd_set, NULL, NULL);
		for(int i = 3; i < MAX_FD; i ++){
			if(FD_ISSET(i, &r_fd_set)){
				if(i == sock_fd){
					int addr_sz = sizeof(cli_addr);
					int cli_fd = accept(sock_fd, (struct sockaddr*)&cli_addr, (socklen_t*)&addr_sz);
					if(cli_fd < 0){
						if(errno == EINTR || errno == EAGAIN) continue;
						ERR_EXIT("accpet");
					}
					printf("getting connect from %s on fd %d.\n", inet_ntoa(cli_addr.sin_addr), cli_fd);
					client[cli_fd].state = CMD;
				} else {
					if(client[i].state == CMD) cmdHandler(client[i], i);
					else if(client[i].state == UPLOAD) uploadHandler(client[i], i);
					else if(client[i].state == STREAM) streamHalter(client[i], i);
				}
			}
			if(FD_ISSET(i, &w_fd_set)){
				if(client[i].state == DOWNLOAD) downloadHandler(client[i], i);
				else if(client[i].state == STREAM) streamBrocaster(client[i], i);
				else if(client[i].state == LS) lsHandler(client[i], i);
			}
		}
	}
}
