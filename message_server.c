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

int user_num = 0;
int sock_list[5];
char user_list[5][50];
char user_Account[5][50] = {"aaa","bbb","ccc","ddd","eee"};
char user_Password[5][50] = {"aaa","bbb","ccc","ddd","eee"};
bool online[5];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *child(void *sock)
{
	int *new_sock = (int*)sock;
	int i,j;
	int index = user_num;
	int account_index;
	char buffer[4096];
	char account[50];
	char password[50];
	char send[4096];
	bool flag = false;
	sock_list[index] = *new_sock;
	printf("Welcome user%d\n",index+1);
	read(sock_list[index],buffer,sizeof(buffer));
	if(strcmp(buffer,"hello") == 0){
		write(*new_sock,"hello",10);
	}
	while(!flag){
		memset(buffer,0,sizeof(buffer));
		memset(account,0,sizeof(account));
		memset(password,0,sizeof(password));
		read(sock_list[index],buffer,sizeof(buffer));
		for(i=0;i<strlen(buffer);i++){
			if(buffer[i] == '$') break;
		}
		strncpy(account,buffer,i);
		account[strlen(account)] = '\0';
		for(j=i+1;j<strlen(buffer);j++){
			password[j-i-1] = buffer[j];
		}
		for(i=0;i<5;i++){
			if(strcmp(account,user_Account[i]) == 0) break;
		}
		account_index = i;
		if(strcmp(password,user_Password[account_index]) == 0){
			pthread_mutex_lock(&mutex);
			strcpy(user_list[user_num],account);
			online[i] = true;
			user_num++;
			pthread_mutex_unlock(&mutex);
			write(sock_list[index],"Sign in",10);
			flag = true;
		}
		else write(*new_sock,"Account or Password wrong",30);
	}
	while(1){
		memset(buffer,0,sizeof(buffer));
		int res = read(sock_list[index],buffer,sizeof(buffer));
		printf("index = %d\nsock = %d\n",index,sock_list[index]);
		if(res <= 0){
			pthread_mutex_lock(&mutex);
			close(sock_list[index]);
			online[account_index] = false;
			user_num--;
			pthread_mutex_unlock(&mutex);
			break;
		}
		else{
			if(strcmp(buffer,"Y") == 0){
				read(sock_list[index],buffer,sizeof(buffer));
				FILE *fp = fopen(buffer,"r");
				if(!fp) printf("Server not found file\n");
				else{
					while(!feof(fp)){
						fread(buffer,sizeof(char),sizeof(buffer),fp);
						write(sock_list[index],buffer,sizeof(buffer));
					}
					write(sock_list[index],"end",5);
				}
				continue;
			}
			printf("user %s request function %s\n",user_list[index],buffer);
			if(strcmp(buffer,"L") == 0){
				strcpy(send,"Online member:\n");
				for(i=0;i<5;i++){
					if(online[i]){
						strcat(send,user_list[i]);
						if(i != user_num - 1){
							strcat(send,"\n");
						}
					}
				}
				write(sock_list[index],send,sizeof(send));
			}
			else if(strcmp(buffer,"SA") == 0){
				strcpy(send,user_list[index]);
				strcat(send," send to all:");
				memset(buffer,0,sizeof(buffer));
				read(sock_list[index],buffer,sizeof(buffer));
				strcat(send,buffer);
				printf("%s\n",send);
				for(i=0;i<user_num;i++){
					if(i == index) continue;
					write(sock_list[i],send,sizeof(send));
				}
			}
			else if(strcmp(buffer,"STO") == 0){
				memset(send,0,sizeof(send));
				memset(buffer,0,sizeof(buffer));
				read(sock_list[index],buffer,sizeof(buffer));
				printf("%s\n",buffer);
				char *ptr = strstr(buffer,"$");
				memset(send,0,sizeof(send));
				strncpy(send,buffer,ptr - &buffer[0]);
				for(i=0;i<5;i++){
					if(strcmp(user_Account[i],send) == 0) break;
				}
				if(i == 5 && !online[i]){
					write(sock_list[index],"Can't find user",20);
				}
				else{
					strcpy(send,user_list[index]);
					strcat(send,":");
					ptr++;
					strcat(send,ptr);
					printf("%s\n",send);
					write(sock_list[i],send,sizeof(send));
				}
			}
			else if(strcmp(buffer,"F") == 0){
				read(sock_list[index],buffer,sizeof(buffer));
				printf("%s\n",buffer);
				char filename[100];
				char *ptr = strstr(buffer,"$");
				memset(send,0,sizeof(send));
				strncpy(send,buffer,ptr - &buffer[0]);
				strcpy(filename,++ptr);
				FILE *fp = fopen(filename,"w");
				if(!fp) printf("Open file error\n");
				while(read(sock_list[index],buffer,sizeof(buffer)) > 0){
					if(strcmp(buffer,"end") == 0){
						fclose(fp);
						break;
					}
					fprintf(fp,"%s\n",buffer);
				}
				for(i=0;i<5;i++){
					printf("%d %s\n",i,send);
					if(strcmp(user_Account[i],send) == 0) break;
				}
				if(i == 5 && !online[i]){
					write(sock_list[index],"Can't find user",20);
				}
				else{
					strcpy(send,user_list[index]);
					strcat(send," want to send file to you. File name:");
					strcat(send,filename);
					strcat(send,"\nAgree? \'Y\' or \'N\'");
					write(sock_list[i],send,sizeof(send));
				}
			}	
			else if(strcmp(buffer,"Q") == 0){
				pthread_mutex_lock(&mutex);
				close(sock_list[index]);
				online[account_index] = false;
				user_num--;
				pthread_mutex_unlock(&mutex);
				printf("user_num:%d\n",user_num);
				break;
			}
			else continue;
		}
	}
	close(sock_list[index]);
	pthread_exit(NULL);
}

int main(int argc,char **argv)
{
	pthread_t t[10];
	int i,yes = 1;
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1){
		printf("Socket create fail\n");
	}
	printf("Server start\n");
	fd_set readset;
	struct sockaddr_in server,client;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(8888);
	int x = bind(sockfd,(struct sockaddr *)&server,sizeof(server));
	if(x == -1){
		perror("bind");
		exit(3);
	}
	x = listen(sockfd,5);
	if(x == -1){
		perror("listen");
		exit(3);
	}
	int size = sizeof(struct sockaddr_in);
	for(i=0;i<5;i++) online[i] = false;
	while(1){
		FD_ZERO(&readset);
		FD_SET(sockfd,&readset); 
		int select_num = select(sockfd + 1,&readset,NULL,NULL,NULL);
		int new_sock;
		if(FD_ISSET(sockfd,&readset)){
			new_sock = accept(sockfd,(struct sockaddr*)&client,(socklen_t*)&size);
			if(new_sock == -1){
				perror("Accept fail\n");
				continue;
			}
			printf("Accept success\n");
			pthread_create(&t[user_num],NULL,child,&new_sock);
		}

	}
	return 0;
}

