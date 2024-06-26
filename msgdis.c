//MESSAGE DISPATCHER
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <sys/resource.h>
#include <time.h>
#include "message.h"

#define TM_REQUEST 0
#define TC_REQUEST 2

//struct_to_receive.val == 0 ACCEPT
//struct_to_receive.val == 1 REJECT

Message send_msg = {0};
Message receive_msg = {0};
int shutdown_type = 0;
mqd_t mqdes_dis, mqdes_tm, mqdes_tc,mqdes_obc;

void *msg_dis(void *arg) {
    mqdes_dis = mq_open("/mq_dispatch", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    mqdes_tm = mq_open("/mq_tm", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    mqdes_tc = mq_open("/mq_tc", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    mqdes_obc = mq_open("/mq_obc", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    
    mqd_t mqdes_type = mq_open("/mq_ttctype", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes); //send to aocs_dispatcher
	  mqd_t mq_return = mq_open("/mq_return_sender", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes); //return from aocs_dispatcher

    if (mqdes_dis == -1 || mqdes_tm == -1 || mqdes_tc == -1) {
        perror("mq_open"); 
        pthread_exit(NULL);
    } 

    while (1) {   
        Message send_msg = {0};
        Message receive_msg = {0}; 
        if (mq_receive(mqdes_dis, (char *)&send_msg, sizeof(send_msg), NULL) == -1) {
            perror("mq_receive"); 
            pthread_exit(NULL);
        }

        if (send_msg.type == TM_REQUEST && send_msg.mdid == 1 && send_msg.req_id > 0 && send_msg.req_id <= 17) {
          printf("- REQUEST TM -\n");
          printf("Type : %u\n", send_msg.type);
          printf("Receive ModuleID : %u\n", send_msg.mdid);
          printf("Receive TelemetryID : %u\n", send_msg.req_id);
          printf("-------------------------------------------\n");
              if (mq_send(mqdes_tm, (char *)&send_msg, sizeof(send_msg), 1) == -1) {
                  perror("mq_send");
                  pthread_exit(NULL);  
              }
              if (mq_receive(mqdes_tm, (char *)&receive_msg, sizeof(receive_msg), NULL) == -1) {
                  perror("mq_receive");
                  pthread_exit(NULL);
              } 
              
              if (receive_msg.type == TM_RETURN) {
                printf("- RETURN TM -\n");
                printf("Type : %u\n", receive_msg.type);
                printf("Respond ModuleID : %u\n", receive_msg.mdid);
                printf("Respond TelemetryID : %u\n", receive_msg.req_id);
                printf("-------------------------------------------\n");
                if (mq_send(mqdes_dis, (char *)&receive_msg, sizeof(receive_msg), 1) == -1) {
                    perror("mq_send");
                    pthread_exit(NULL);
                }
            }  
            else {
            printf("- REQUEST NO TYPE -\n");
            printf("Type : %u\n", send_msg.type);
            printf("Receive ModuleID : %u\n", send_msg.mdid);
            printf("Receive TelemetryID : %u\n", send_msg.req_id);
            printf("No Type\n"); 
            printf("-------------------------------------------\n");
            if (mq_send(mqdes_dis, (char *)&receive_msg, sizeof(receive_msg), 1) == -1) {
                  perror("mq_send");
                  pthread_exit(NULL);
            } 
          }       
        }
        else if (send_msg.type == TM_REQUEST && send_msg.mdid == 2 && send_msg.req_id != 0) {
          printf("- REQUEST TM -\n");
          printf("Type : %u\n", send_msg.type);
          printf("Receive ModuleID : %u\n", send_msg.mdid);
          printf("Receive TelemetryID : %u\n", send_msg.req_id);
          printf("-------------------------------------------\n");
            if (mq_send(mqdes_type, (char *)&send_msg, sizeof(send_msg), 1) == -1) {
                perror("mq_send");
                pthread_exit(NULL);
            }
            if (mq_receive(mq_return, (char *)&receive_msg, sizeof(receive_msg), NULL) == -1) {
                perror("mq_receive");
                pthread_exit(NULL);
            }
            if (receive_msg.type == TM_RETURN) {
              printf("- RETURN TM -\n");
              printf("Type : %u\n", receive_msg.type);
              printf("Respond ModuleID : %u\n", receive_msg.mdid);
              printf("Respond TelemetryID : %u\n", receive_msg.req_id);
              printf("-------------------------------------------\n");
              if (mq_send(mqdes_dis, (char *)&receive_msg, sizeof(receive_msg), 1) == -1) {
                  perror("mq_send");
                  pthread_exit(NULL);
              }
            }         
        }
        
        else if (send_msg.type == TC_REQUEST && send_msg.mdid == 1 && send_msg.req_id > 0 && send_msg.req_id <= 5) {
           
          printf("- REQUEST TC -\n");
          printf("Type : %u\n", send_msg.type);
          printf("Receive ModuleID : %u\n", send_msg.mdid);
          printf("Receive TelemetryID : %u\n", send_msg.req_id);
          printf("-------------------------------------------\n");
            if (mq_send(mqdes_tc, (char *)&send_msg, sizeof(send_msg), 1) == -1) {
                perror("mq_send");
                pthread_exit(NULL);
            }
            if (mq_receive(mqdes_tc, (char *)&receive_msg, sizeof(receive_msg), NULL) == -1) {
                perror("mq_receive"); 
                pthread_exit(NULL);
            }
                            
            if (receive_msg.type == TC_RETURN  && receive_msg.val == 1) {
              printf("- REJECT TC -\n");
              printf("Type : %u\n", receive_msg.type);
              printf("Respond ModuleID : %u\n", receive_msg.mdid);
              printf("Respond TelemetryID : %u\n", receive_msg.req_id);
              printf("-------------------------------------------\n");
              if (mq_send(mqdes_dis, (char *)&receive_msg, sizeof(receive_msg), 1) == -1) {
                  perror("mq_send");
                  pthread_exit(NULL);
              }
            }  
                   
            else if (receive_msg.type == TC_RETURN ) {
              printf("- RETURN TC -\n");
              printf("Type : %u\n", receive_msg.type);
              printf("Respond ModuleID : %u\n", receive_msg.mdid);
              printf("Respond TelemetryID : %u\n", receive_msg.req_id);
              printf("-------------------------------------------\n");
              if (mq_send(mqdes_dis, (char *)&receive_msg, sizeof(receive_msg), 1) == -1) {
                  perror("mq_send");
                  pthread_exit(NULL);
              }  
            }  
        }
        else {
            printf("- REQUEST NO TYPE -\n");
            printf("Type : %u\n", send_msg.type);
            printf("Receive ModuleID : %u\n", send_msg.mdid);
            printf("Receive TelemetryID : %u\n", send_msg.req_id);
            printf("No Type\n"); 
            printf("-------------------------------------------\n");
            if (mq_send(mqdes_dis, (char *)&receive_msg, sizeof(receive_msg), 1) == -1) {
                  perror("mq_send");
                  pthread_exit(NULL);
            } 
        } 
    }  

    mq_close(mqdes_dis);  
    mq_close(mqdes_tm);
    mq_close(mqdes_tc);
    mq_unlink("/mq_dispatch");
    mq_unlink("/mq_tm");
    mq_unlink("/mq_tc");
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    pthread_t msg_dis1;

    pthread_create(&msg_dis1, NULL, msg_dis, NULL);
    pthread_join(msg_dis1, NULL); 

    return 0;
} 