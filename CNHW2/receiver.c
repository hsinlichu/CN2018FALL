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
    int agentsocket;
    struct sockaddr_in agent,receiver, tmp_addr;
    socklen_t recv_size, agent_size, tmp_size;
    char ip[2][50]; // ip[0]: agent; ip[1]: sender
    int port[2];
    char filename[50];

    if(argc != 5){
        fprintf(stderr,"用法: %s <agent IP> <agent port> <receiver port> <output filename>\n", argv[0]);
        fprintf(stderr, "例如: ./agent local 8887 8888 ./output.csv\n");
        exit(1);
    } else {
        setIP(ip[0], argv[1]);
        setIP(ip[1], "local");

        sscanf(argv[2], "%d", &port[0]);
        sscanf(argv[3], "%d", &port[1]);

        strcpy(filename,argv[4]);
        printf("filename: %s\n",filename);
    }
    FILE *fp = fopen(filename,"wb");
    assert(fp != NULL);

    /*Create UDP socket*/
    agentsocket = socket(PF_INET, SOCK_DGRAM, 0);

    /*Configure settings in agent struct*/
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[0]);
    agent.sin_addr.s_addr = inet_addr(ip[0]);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

    /*Configure settings in receiver struct*/
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port[1]);
    receiver.sin_addr.s_addr = inet_addr(ip[1]);
    memset(receiver.sin_zero, '\0', sizeof(receiver.sin_zero)); 

    /*bind socket*/
    bind(agentsocket,(struct sockaddr *)&receiver,sizeof(receiver));

    /*Initialize size variable to be used later on*/
    agent_size = sizeof(agent);
    tmp_size = sizeof(tmp_addr);
    recv_size = sizeof(receiver);

    printf("agent info: ip = %s port = %d | sender info: ip = %s port = %d\n", ip[0], port[0], ip[1], port[1]);

    int segment_size;
    int seg_len = 1000;
    printf("seg_len: %d\n",seg_len);

    char **buffer = (char**)malloc(32*sizeof(char*));
    int *data_len = (int*)malloc(32*sizeof(int));
	for(int i = 0;i < 32;++i)
		buffer[i] = (char*)malloc(seg_len*sizeof(char));
	int recv_seq = 0; // the sequence number of received segment
	int num_store = 0;
	int recvfin = 0;
    while(1){
        /*Receive message from receiver and sender*/
		segment s_tmp,acksegment;
        memset(&s_tmp, 0, sizeof(s_tmp));
        segment_size = recvfrom(agentsocket, &s_tmp, sizeof(s_tmp), 0, (struct sockaddr *)&tmp_addr, &tmp_size);
        if(segment_size > 0){
			if(num_store == 32){
				printf("drop    data    #%d\n",s_tmp.head.seqNumber);
				for(int i = 0;i < num_store;++i)
					fwrite(buffer[i],data_len[i],1,fp);
				num_store = 0;
				printf("flush\n");
				continue;
			}
			else{ 
				if(s_tmp.head.fin && (recv_seq + 1 == s_tmp.head.seqNumber)){
					printf("recv    fin\n");
					recvfin	= 1;
					recv_seq++;
				}
				else{
					//printf("last recv seq num %d | now recv %d\n",recv_seq,s_tmp.head.seqNumber);
					if(recv_seq + 1 == s_tmp.head.seqNumber){ // recive inorder segment
						recv_seq++;
						printf("recv    data    #%d\n",s_tmp.head.seqNumber);
						data_len[num_store] = s_tmp.head.length;
						//printf("store in num_store %d len %d\n",num_store,data_len[num_store]);
						memcpy(buffer[num_store],s_tmp.data,data_len[num_store]);
						num_store++;
					}
					else{
						printf("drop    data    #%d\n",s_tmp.head.seqNumber);
				    // 	printf("duplicate package or out of order package\n");
					}
				}
			}
			memset(&acksegment, 0, sizeof(acksegment));
						
            acksegment.head = (header){.length = 0 ,.seqNumber = 0,.ackNumber = recv_seq,.fin = 0,.syn = 0,.ack = 1};
			if(recvfin){
				acksegment.head.fin = 1;
			}
            //printheader(&acksegment);
            sendto(agentsocket, &acksegment,sizeof(acksegment), 0, (struct sockaddr *)&agent, agent_size);
			if(recvfin){
				printf("send    finack\n");
				for(int i = 0;i < num_store;++i)
					fwrite(buffer[i],data_len[i],1,fp);
				num_store = 0;
				printf("flush\n");
				break;
			}
			else
				printf("send    ack     #%d\n",recv_seq);
        }
    }

	fclose(fp);
	for(int i = 0;i < 32;++i)
		free(buffer[i]);
	free(buffer);
	free(data_len);
    return 0;
}
