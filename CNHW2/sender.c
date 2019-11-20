/* NOTE: 本程式僅供檢驗格式，並未檢查 congestion control 與 go back N */
/* NOTE: UDP socket is connectionless，每次傳送訊息都要指定ip與埠口 */
/* HINT: 建議使用 sys/socket.h 中的 bind, sendto, recvfrom */

/*
 * 連線規範：
 * 本次作業之 agent, sender, receiver 都會綁定(bind)一個 UDP socket 於各自的埠口，用來接收訊息。
 * agent接收訊息時，以發信的位址與埠口來辨認訊息來自sender還是receiver，同學們則無需作此判斷，因為所有訊息必定來自agent。
 * 發送訊息時，sender & receiver 必需預先知道 agent 的位址與埠口，然後用先前綁定之 socket 傳訊給 agent 即可。
 * (如 agent.c 第126行)
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

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
	char data[1000];
} segment;

void printheader(segment *tmp){
    printf("length %d | seqNumber %d | acknumber %d | fin %d | syn %d | ack %d\n",
            tmp->head.length,tmp->head.seqNumber,tmp->head.ackNumber,tmp->head.fin,tmp->head.syn,tmp->head.ack);
    return;
}

void setIP(char *dst, char *src) {
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0 || strcmp(src, "localhost")) {
        sscanf("127.0.0.1", "%s", dst);
    } else {
        sscanf(src, "%s", dst);
    }
}

int main(int argc, char* argv[]){
    int agentsocket, portNum, nBytes;
    float loss_rate;
    struct sockaddr_in agent,sender, tmp_addr;
    socklen_t sender_size, agent_size, tmp_size;
    char ip[2][50]; // ip[0]: agent; ip[1]: sender
    int port[2];
    char filename[50];

    if(argc != 5){
        fprintf(stderr,"用法: %s <agent IP> <agent port> <sender port> <filename>\n", argv[0]);
        fprintf(stderr, "例如: ./sender local 8887 8888 ./data.csv\n");
        exit(1);
    } else {
        setIP(ip[0], argv[1]);
        setIP(ip[1], "local");

        sscanf(argv[2], "%d", &port[0]);
        sscanf(argv[3], "%d", &port[1]);

        strcpy(filename,argv[4]);
        printf("filename: %s\n",filename);
    }
    FILE *fp = fopen(filename,"rb");
    assert(fp != NULL);

    /*Create UDP socket*/
    agentsocket = socket(PF_INET, SOCK_DGRAM, 0);
	int flags = fcntl(agentsocket,F_GETFL,0);
	fcntl(agentsocket,F_SETFL,flags | O_NONBLOCK);

    /*Configure settings in agent struct*/
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[0]);
    agent.sin_addr.s_addr = inet_addr(ip[0]);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

    /*Configure settings in sender struct*/
    sender.sin_family = AF_INET;
    sender.sin_port = htons(port[1]);
    sender.sin_addr.s_addr = inet_addr(ip[1]);
    memset(sender.sin_zero, '\0', sizeof(sender.sin_zero));  

    /*bind socket*/
    bind(agentsocket,(struct sockaddr *)&sender,sizeof(sender));

    /*Initialize size variable to be used later on*/
    sender_size = sizeof(sender);
    agent_size = sizeof(agent);
    tmp_size = sizeof(tmp_addr);

    printf("agent info: ip = %s port = %d | sender info: ip = %s port = %d\n", ip[0], port[0], ip[1], port[1]);

    int segment_size;
    int windows_size = 1;
    int threshold = 16;
    double timeout = 1;
    int send_base = 1;
    int recvfinack = 0;
    int retransmit = 0;
       

    int seg_len = 1000;
    //printf("seg_len: %d\n",seg_len);
    char **data = (char**)malloc(10*1024*sizeof(char*));
    int *data_len = (int*)malloc(10*1024*sizeof(int));
    int segment_num = 1;
    int last_segment_len = 0;
	int ret;
    while(1){
        char *tmp = (char*)malloc(1000*sizeof(char));
        ret = fread(tmp,sizeof(char),seg_len,fp);
		//printf("ret %d\n",ret);
        data[segment_num] = tmp;
		data_len[segment_num] = ret;
		segment_num++;
        if(ret < 1000){
            last_segment_len = ret;
            break;
		}
    }
    printf("segment number %d | last segment length %d\n",segment_num - 1,last_segment_len);
    clock_t* time = (clock_t*)malloc((segment_num + 1) * sizeof(clock_t));
    int* transmit_times = (int*)malloc((segment_num + 1) * sizeof(int));
	memset(transmit_times,0,(segment_num + 1) * sizeof(int));

    int from = send_base;
    int to = send_base + windows_size;
    while(!recvfinack){
        for(int i = from;i < to && i <= segment_num;++i){
			//printf("i %d\n",i);
            segment s_tmp;
            memset(&s_tmp, 0, sizeof(s_tmp));
			if(i == segment_num){ // fin
				s_tmp.head = (header){.length = 0 ,.seqNumber = i,.ackNumber = 0,.fin = 1,.syn = 0,.ack = 0};
				printf("send     fin\n");
			}
			else{
				//printf("data_len %d\n",data_len[i]);
				memcpy(s_tmp.data,data[i],data_len[i]);
				s_tmp.head = (header){.length = data_len[i] ,.seqNumber = i,.ackNumber = 0,.fin = 0,.syn = 0,.ack = 0};
				char* format = (transmit_times[i] > 0) ? "resnd    data    #%4d,    winSize   = %2d\n":"send     data    #%4d,    winSize   = %2d\n";
				printf(format,i,windows_size);
			}
            //printheader(&s_tmp);
            sendto(agentsocket, &s_tmp,sizeof(s_tmp), 0, (struct sockaddr *)&agent, agent_size);
			transmit_times[i] += 1;
			time[i] = -clock();
        }
        int waitack = from;
        segment recv;
        while(1){
			double check = (double)(time[waitack] + clock()) / CLOCKS_PER_SEC;
			//printf("%lf\n",check);
            if(check >= timeout){ // timeout
				retransmit = 1;
                from = waitack;
                int tmp = windows_size / 2;
                threshold = (tmp > 1) ? tmp : 1;
                windows_size = 1;
                from = waitack;
                to = from + windows_size;
				printf("time     out,              threshold = %2d\n",threshold);
				//printf("wait for %d\n",waitack);

                break;
            }
            memset(&recv, 0, sizeof(recv));
            segment_size = recvfrom(agentsocket, &recv, sizeof(recv),MSG_DONTWAIT, (struct sockaddr *)&tmp_addr, &tmp_size);
            if(segment_size > 0){
				if(waitack == recv.head.ackNumber && recv.head.fin){
					printf("recv    finack\n");
					recvfinack = 1;
					waitack = recv.head.ackNumber + 1;
					break;
				}
                if(recv.head.ack){
					waitack = recv.head.ackNumber + 1;
					printf("recv     ack     #%4d\n",recv.head.ackNumber);
				}
				
            }
            if(waitack == to){
                from = to;
                if(windows_size >= threshold)
                    windows_size += 1;
                else
                    windows_size *= 2;
                to += windows_size;
                retransmit = 0;
				//printf("Get all ack | from %d | to %d | retransmit %d\n",from,to,retransmit);
                break;
            }
        }
    }
	fclose(fp);
	for(int i = 1;i < segment_num;++i)
		free(data[i]);
	free(data);
	free(data_len);
	free(transmit_times);
    return 0;
}
