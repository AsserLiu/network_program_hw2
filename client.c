#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/select.h>
#include<sys/socket.h>
#include<time.h>
#include<string.h>
#include<netinet/in.h>
#include<errno.h>
#include<fcntl.h>

int main(int argc,char **argv)
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1){
		printf("Socket create fail\n");
	}
	printf("Cilent start\n");
	struct sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	client.sin_port = htons(8888);
	int x = connect(sockfd,(struct sockaddr *)&client,sizeof(client));
	char head[5] = "hello";
	write(sockfd,head,sizeof(head));
	char buffer[4096];
	char account[50];
	char password[50];
	read(sockfd,buffer,sizeof(buffer));
	if(strcmp(buffer,"hello") == 0){
		bool flag = false;
		while(!flag){
			printf("Account :");
			fgets(account,sizeof(account),stdin);
			account[strlen(account) - 1] = '\0';
			strcat(account,"$");
			printf("Password :");
			fgets(password,sizeof(password),stdin);
			password[strlen(password) - 1] = '\0';
			memset(buffer,0,sizeof(buffer));
			strcpy(buffer,account);
			strcat(buffer,password);
			printf("buffer = %s\n",buffer);
			write(sockfd,buffer,sizeof(buffer));
			memset(buffer,0,sizeof(buffer));
			read(sockfd,buffer,sizeof(buffer));
			if(strcmp(buffer,"Sign in") == 0){
				printf("Sign in\n");
				flag = true;
			}
			else printf("Account or Password wrong\n");
		}
		printf("Fuction info\nL:list all online user\nSA:send message to all user\nSTO:send message to one user\nF:send file to one user\nQ:Quit\n");
		fd_set readfd;
		while(1){
			char function[10];
			memset(function,0,sizeof(function));
			memset(buffer,0,sizeof(buffer));
			FD_ZERO(&readfd);
			FD_SET(0,&readfd);
			FD_SET(sockfd,&readfd);
			select(sockfd + 1,&readfd,NULL,NULL,NULL);
			if(FD_ISSET(0,&readfd)){
				read(0,function,sizeof(function));
				function[strlen(function) - 1] = '\0';
			}
			if(FD_ISSET(sockfd,&readfd)){
				read(sockfd,buffer,sizeof(buffer));
				printf("%s\n",buffer);
				if(strstr(buffer,"send file to") != NULL){
					char c;
					scanf("%c",&c);
					if(c == 'Y'){
						write(sockfd,"Y",2);
						char *ptr = strstr(buffer,"File name:");
						ptr = strstr(ptr,":");
						char filename[100];
						char *qtr = strstr(++ptr,"Agree");
						qtr--;
						memset(filename,0,sizeof(filename));
						strncpy(filename,ptr,qtr - ptr);
						printf("filename = %s\n",filename);
						write(sockfd,filename,sizeof(filename));
						FILE *fp = fopen(filename,"w");
						while(read(sockfd,buffer,sizeof(buffer)) >0){
							if(strcmp(buffer,"end") == 0){
								fclose(fp);
								break;
							}
							printf("from = %s\n",buffer);
							fprintf(fp,"%s",buffer);
						}
					}
				}
				continue;
			}
			if(strcmp(function,"L") == 0){
				write(sockfd,"L",3);
				memset(buffer,0,sizeof(buffer));
				read(sockfd,buffer,sizeof(buffer));
				printf("%s\n",buffer);
			}
			else if(strcmp(function,"SA") == 0){
				memset(buffer,0,sizeof(buffer));
				printf("Write message:");
				fgets(buffer,sizeof(buffer),stdin);
				write(sockfd,"SA",10);	
				write(sockfd,buffer,sizeof(buffer));
			}
			else if(strcmp(function,"STO") == 0){
				write(sockfd,"STO",10);
				memset(buffer,0,sizeof(buffer));
				printf("Who?");
				char send[4096];
				memset(send,0,sizeof(send));
				fgets(send,sizeof(send),stdin);
				send[strlen(send) - 1] = '$';
				printf("Write message:");
				fgets(buffer,sizeof(buffer),stdin);
				strcat(send,buffer);
				printf("send = %s\n",send);
				write(sockfd,send,sizeof(send));
			}
			else if(strcmp(function,"F") == 0){
				write(sockfd,"F",10);
				char send[4096];
				memset(send,0,sizeof(send));
				memset(buffer,0,sizeof(buffer));
				printf("Who?");
				fgets(send,sizeof(send),stdin);
				send[strlen(send) - 1] = '$';
				printf("File name:");
				fgets(buffer,sizeof(buffer),stdin);
				buffer[strlen(buffer) - 1] = '\0';
				FILE *fp = fopen(buffer,"r");
				while(!fp){
					printf("File not found\n");
					memset(buffer,0,sizeof(buffer));
					fgets(buffer,sizeof(buffer),stdin);
					fp = fopen(buffer,"r");
				}
				strcat(send,buffer);
				write(sockfd,send,sizeof(send));
				while(!feof(fp)){
					fread(buffer,sizeof(char),sizeof(buffer),fp);
					write(sockfd,buffer,sizeof(buffer));
				}
				write(sockfd,"end",5);
			}
			else if(strcmp(function,"Q") == 0){
				write(sockfd,"Q",10);
				sleep(1);
				break;
			}
			else{
				printf("Fuction not found\n");
				continue;
			}
		}
	}
	close(sockfd);
	return 0;
}
