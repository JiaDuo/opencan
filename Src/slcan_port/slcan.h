#ifndef SLCAN_H 
#define SLCAN_H
#include "string.h"
#include "stdio.h"
#include "stdint.h"
#include "usbd_cdc_if.h"
#include "main.h"
//reference SLCAN-API.pdf   CAN232_v3.pdf

#define CMD_BUF_MAX  50

#define CMD_CR '\r'   //[CR]    End of Command
#define CMD_ACK "\r"  //[CR]
#define CMD_NOACK '\a' 
#define CMD_BELL    7

#define CMD_SET_BITRATE 'S'
#define CMD_SET_BTR 's'
#define CMD_OPEN    'O'
#define CMD_CLOSE   'C'

#define CMD_SFF     't' //TX/RX Frame Format SFF
#define CMD_EFF     'T' //TX/RX Frame Format EFF
#define CMD_RSFF    'r' //TX/RX Frame Format RTR/SFF
#define CMD_REFF    'R' //TX/RX Frame Format RTR/EFF

#define CMD_RSF         'F' //Read Status Flags
#define CMD_TIMESTAMP   'Z' //Timestamp On/Off

#define CMD_ACCEPT_MASK     'M' //Acceptance Mask
#define CMD_ACCEPT_VALUE    'm' //Acceptance Value

#define CMD_VER     'V'
#define CMD_SN      'N'

typedef struct{
    char cmd;
    char speed;
}SetupBitrate_def;

typedef struct{
    char cmd;
    char btr0[2];
    char btr1[2];
}SetupBtr_def;

typedef struct{
    char cmd;
    char timestamp;
}TimeStamp_def;

typedef struct{
    char cmd;
    char v1[2];
    char v2[2];
    char v3[2];
    char v4[2];
}Acceptance_def;
typedef Acceptance_def AcceptanceMask_def;
typedef Acceptance_def AcceptanceValue_def;

typedef struct{
    char cmd;
    char id[3];
    char len;
    char data[20]; //8 bytes + [CR] ,>=17
}StandFrame_def;

typedef struct{
    char cmd;
    char id[8];
    char len;
    char data[20];
}ExtendFrame_def;

typedef struct{
    char cmd;
    char id[3];
    char len;
    char cr;
}RemoteStandFrame_def;

typedef struct{
    char cmd;
    char id[8];
    char len;
    char cr;
}RemoteExtendFrame_def;

void onlytest(char *);
int slcan_cdc_process(char *st,int len);

uint32_t hex2int(char *hex,uint8_t len);
void int2hex(uint32_t data,int len,char * out);

#endif