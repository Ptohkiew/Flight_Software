#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <mqueue.h>


typedef struct {
    uint8_t type;
    //type = 0 TM_Request
    //type = 1 TM_Return
    //type = 2 TC_Request
    //type = 3 TC_Return
    #define TM_REQUEST   0
    #define TM_RETURN    1
    #define TC_REQUEST   2
    #define TC_RETURN    3
    uint32_t mdid;
    uint32_t req_id;
    uint32_t param;
    uint32_t val;
} Message;

static struct mq_attr attributes = {
    .mq_flags = 0,
    .mq_maxmsg = 10,
    .mq_curmsgs = 0,
    .mq_msgsize = sizeof(Message)
};

#define STANDBY  0
#define REBOOT   1
#define SHUTDOWN 2

#endif // MESSAGE_H
  