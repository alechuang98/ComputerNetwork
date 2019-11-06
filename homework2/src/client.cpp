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
#include <ctype.h>
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
}
void writer(int fd, char *buf, int size){
	int p = 0;
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

int initClient(char *ip, unsigned short port){
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = inet_addr(ip);
	sockaddr.sin_port = htons(port);

	int tmp = 1;
	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0){
		ERR_EXIT("setsockopt");
	}
	if(connect(sock_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0){
		ERR_EXIT("bind");
	}
	return sock_fd;
}

char buf[MAX_BUF], file_name[HEADER_SIZE];
void lsHandler(node &client, int fd){
	reader(fd, buf, BUF_SIZE);
	printf("%s\n", buf);
}
void downloadHandler(node &cli, int fd){
	while(cli.size > 0){
		reader(fd, buf, BUF_SIZE);
		fwrite(buf, sizeof(char), min(BUF_SIZE, cli.size), cli.fp);
		cli.size -= min(cli.size, BUF_SIZE);
	}
	fflush(cli.fp);
	fclose(cli.fp);
}
void uploadHandler(node &cli, int fd){
	sprintf(buf, "%d", cli.size);
	writer(fd, buf, HEADER_SIZE);
	while(cli.size > 0){
		fread(buf, sizeof(char), min(BUF_SIZE, cli.size), cli.fp);
		writer(fd, buf, BUF_SIZE);
		cli.size -= min(cli.size, BUF_SIZE);
	}
	fclose(cli.fp);
}
void streamHandler(node &cli, int fd){
	bool end = 0;
	while(!end){
		reader(fd, buf, cli.size);
		if(strcmp(buf, END_TAG) == 0) end = 1;
		uchar *ptr = cli.img.data;
		memcpy(ptr, buf, cli.size);
		imshow("Video", cli.img);
		char c = (char)waitKey(33.3333);
		if(c == 27) break;
	}
	destroyAllWindows();
	// clear buffer
	strcpy(buf, END_TAG);
	writer(fd, buf, HEADER_SIZE);
	while(!end){
		reader(fd, buf, cli.size);
		if(strcmp(buf, END_TAG) == 0) end = 1;
	}
}

int main(int argc, char *argv[]){
	char *ip = strtok(argv[1], ":");
	char *port = strtok(NULL, ":");
	int sock_fd = initClient(ip, atoi(port));
	node client;
	string inp;
	while(1){
		printf("$ ");
		cin.getline(buf, HEADER_SIZE);
		string cmd = string(buf);
		if(cmd.substr(0, 2) != "ls" && cmd.substr(0, 3) != "get" && cmd.substr(0, 3) != "put"
			&& cmd.substr(0, 4) != "play"){
			puts("Command not found.");
			continue;
		}
		int space_count = count(cmd.begin(), cmd.end(), ' ');
		if(space_count > 1 || (space_count == 0 && cmd != "ls") &&
			space_count >= 1 && cmd.substr(0, 2) == "ls"){
			puts("Command format error.");
			continue;
		}
		if(cmd == "ls"){
			writer(sock_fd, buf, HEADER_SIZE);
			lsHandler(client, sock_fd);
		} else if(cmd.substr(0, 4) == "get "){
			writer(sock_fd, buf, HEADER_SIZE);
			client.size = 0;
			strcpy(file_name, cmd.substr(4).c_str());
			reader(sock_fd, buf, HEADER_SIZE);
			if(!isdigit(buf[0])){
				puts(buf);
			} else {
				client.fp = fopen(file_name, "wb");
				client.size = atoi(buf);
				downloadHandler(client, sock_fd);
			}
		} else if(cmd.substr(0, 4) == "put "){
			client.size = 0;
			strcpy(file_name, cmd.substr(4).c_str());
			if(access(file_name, F_OK) == -1){
				printf("The %s doesn't exist.\n", file_name);
			} else {
				writer(sock_fd, buf, HEADER_SIZE);
				client.fp = fopen(file_name, "rb");
				client.size = getFileLen(client.fp);
				uploadHandler(client, sock_fd);
			}
		} else if(cmd.substr(0, 5) == "play "){
			writer(sock_fd, buf, HEADER_SIZE);
			reader(sock_fd, buf, HEADER_SIZE);
			if(!isdigit(buf[0])){
				puts(buf);
			} else {
				int h, w;
				sscanf(buf, "%d %d", &h, &w);
				printf("h: %d w: %d\n", h, w);
				client.img = Mat::zeros(h, w, CV_8UC3);
				if(!client.img.isContinuous()){
					client.img = client.img.clone();
				}
				client.size = client.img.total() * client.img.elemSize();
				streamHandler(client, sock_fd);
			}
		} else {
			puts("Command not found.");
		}
	}
}
