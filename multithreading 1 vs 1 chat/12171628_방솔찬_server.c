#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include<stdbool.h>


#define BUF_SIZE 100
#define MAX_CLNT 256
#define NAME_SIZE 20

void* handle_clnt(void* arg);
void send_msg(char* msg, int len, int clnt_sock);
void error_handling(char* msg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];

//char* clnt_socks_name[MAX_CLNT];

pthread_mutex_t mutx;
char name_msg[BUF_SIZE]; //save the name and message

int serv_sock, clnt_sock;
void clearFunc(char msg[]);//clear all buffer 



char** clnt_socks_name;//save client socket name 

int main(int argc, char* argv[])
{
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)//error handle
		error_handling("bind() error");

	if (listen(serv_sock, 5) == -1)//error handle 
		error_handling("listen() error");

	clnt_socks_name = (char**)malloc(sizeof(char*) * MAX_CLNT);// set the size of socks name 
	for (int i = 0; i < MAX_CLNT; i++) {
		clnt_socks_name[i] = (char*)malloc(sizeof(char) * MAX_CLNT);
	}



	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

		//printf("clnt_sock:%d\n", clnt_sock);



		clearFunc(name_msg);
		int str_len = read(clnt_sock, name_msg, BUF_SIZE);//read the client sockets name 
		//printf("%s\n", name_msg);


		if (str_len == -1) {
			error_handling("read() error!");
		}


		name_msg[str_len] = 0;
		//fputs(name_msg, stdout);
		printf("user @%s has entered the chat\n", name_msg);//print enter the chat when entered 
		//clnt_socks_name[clnt_sock] = name_msg;

		//strncpy(clnt_socks_name[clnt_sock],name_msg, BUF_SIZE);

		for (int i = 0; i < str_len; i++) {//save the client sockets name 
			clnt_socks_name[clnt_sock][i] = name_msg[i];
		}
		clnt_socks_name[clnt_sock][str_len] = 0;//end the buffer 

		//printf("clientsock 4th name %s\n", clnt_socks_name[4]);
		//printf("clientsock 5th name %s\n", clnt_socks_name[5]);
		//printf("clientsock 6th name %s\n", clnt_socks_name[6]);

		//printf("clnt_sock:%d   name_msg:%s\n", clnt_sock, name_msg);

		//printf("%c\n",clnt_socks_name[clnt_sock][6]);

		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++] = clnt_sock;
		pthread_mutex_unlock(&mutx);

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		//printf("Connected client IP: %s \n" , inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}

void* handle_clnt(void* arg)
{
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE] = "";

	while (1) {

		//	printf("clientsocket %d\n", clnt_sock);
			//printf("while %s\n", msg);
		if (str_len = read(clnt_sock, msg, sizeof(msg)) == 0) {//read the client buffers data 
			break;
		}
		//	printf("before send_msg clnt_sock: %d\n", clnt_sock);
		send_msg(msg, str_len, clnt_sock); //send msg to clnt_sock 
		//printf("after send_msg clnt_sock: %d\n", clnt_sock);
	}

	pthread_mutex_lock(&mutx);
	for (i = 0; i < clnt_cnt; i++)   // remove disconnected client
	{
		if (clnt_sock == clnt_socks[i])
		{
			//printf("%d\n", clnt_sock);
			printf("user @%s has left the chat\n", clnt_socks_name[clnt_sock]);//when client socket is closed print message 
			clnt_socks_name[clnt_sock] = 0;
			while (i < clnt_cnt - 1)
			{
				clnt_socks[i] = clnt_socks[i + 1];
				i++;
			}
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}
void clearFunc(char* msg) {//clear all buffer 
	for (int i = 0; i < BUF_SIZE; i++) {
		msg[i] = 0;
	}
}
void send_msg(char* msg, int len, int clnt_sock)
{
	//printf("send start\n");
	printf("msg: %s", msg);
	int i;
	pthread_mutex_lock(&mutx);
	if (msg[1] == 'a' && msg[2] == 'l' && msg[3] == 'l') { // send to all
		char name_msg[NAME_SIZE + BUF_SIZE];
		char* Sender_Name = clnt_socks_name[clnt_sock];

		for (int i = 0; i < strlen(msg)-4; i++) {//message change 
			msg[i] = msg[i + 4];
		}
		int s = strlen(msg);
		msg[s - 4] = 0;

		sprintf(name_msg, "[%s] %s", Sender_Name, msg);
		for (i = 0; i < clnt_cnt; i++) {
			//printf("clnt_cnt: %d\n", clnt_cnt);
			write(clnt_socks[i], name_msg, strlen(name_msg));

		}
		//printf("\n");
	}
	else {//1 vs 1 chat 
		char ReceiverName[NAME_SIZE];
		int i = 1;
		for (; i < strlen(msg); i++) {
			if (msg[i] == ' ') {
				ReceiverName[i - 1] = 0;
				break;
			}
			ReceiverName[i - 1] = msg[i];
		}
		//printf("ReceiverName:%s\n", ReceiverName);

		int msg_size = strlen(msg);
		for (int j = 0; j < msg_size; j++) {
			if (j + i + 1 == msg_size) {
				msg[j] = 0;
				break;
			}
			msg[j] = msg[j + i];
		}
	

		char* sendername = clnt_socks_name[clnt_sock];//client sock은 보낸사람 sock 
		//printf("sendername:%s\n", sendername);

		sprintf(name_msg, "[%s] %s\n", sendername, msg);
		//printf("name_msg: %s", name_msg);

		int receiver_sock;
		bool get_Name = false;


		for (int i = 0; i < clnt_cnt; i++) {

			if (strlen(clnt_socks_name[i+4])==strlen(ReceiverName)) {
				bool nameCorrect = true;//if name is in the socket_name  namecorrect is true ,else false 
				for (int j = 0; j< strlen(ReceiverName); j++) {
					if (clnt_socks_name[i+4][j] != ReceiverName[j]) {
						nameCorrect = false;
						break;
					}
				}
				if (nameCorrect == true) {
					//printf("get_Name==TRUE\n");
					get_Name = true;
					
					write(clnt_socks[i], name_msg, strlen(name_msg));
					
					break;
				}
				
				
			}

		}

		if (!get_Name) {//if their is no target user 
			char* No_Target = "Target user not found! \n";
			write(clnt_sock, No_Target, strlen(No_Target));
		}

	}
	//printf("clearfunc before\n");
	clearFunc(msg);
	//printf("clearfunc end\n");
	pthread_mutex_unlock(&mutx);

}
void error_handling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}