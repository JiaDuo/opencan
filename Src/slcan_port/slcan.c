#include "slcan.h"



char cmd_buf[CMD_BUF_MAX]={0};
int	cmd_buf_off = 0;

/* hcan.Init.Prescaler value 8=1Mbps*/
const uint32_t bitrate_map[9]={800,400,160,80,64,32,16,10,8};
const char hex_str_map[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

/* slcan response function */
#define cmd_response(buf,len) {\
	CDC_Transmit_FS((uint8_t*)buf,len);\
}

/* rx stand frame to cmd str */
void sff_to_cmd(CAN_RxHeaderTypeDef* header,uint8_t data[])
{
	StandFrame_def cmd_frame;
	uint32_t cmd_len;
	cmd_frame.cmd = CMD_SFF;
	int2hex(header->StdId,3,cmd_frame.id);
	int2hex(header->DLC,1,&cmd_frame.len);
	for(cmd_len = 0;cmd_len < header->DLC;cmd_len++)
	{
		int2hex(data[cmd_len],2,cmd_frame.data+(cmd_len<<1));
	}
	cmd_len = ((header->DLC)<<1) + 5 + 1;
	((uint8_t*)&cmd_frame)[cmd_len-1] = CMD_CR;
	cmd_response(&cmd_frame,cmd_len);
}

/* rx extend frame to cmd str */
void eff_to_cmd(CAN_RxHeaderTypeDef* header,uint8_t data[])
{
	ExtendFrame_def cmd_frame;
	uint32_t cmd_len;
	cmd_frame.cmd = CMD_EFF;
	int2hex(header->ExtId,8,cmd_frame.id);
	int2hex(header->DLC,1,&cmd_frame.len);
	for(cmd_len = 0;cmd_len < header->DLC;cmd_len++)
	{
		int2hex(data[cmd_len],2,cmd_frame.data+(cmd_len<<1));
	}
	cmd_len = ((header->DLC)<<1) + 10 + 1;
	((uint8_t*)&cmd_frame)[cmd_len-1] = CMD_CR;
	cmd_response(&cmd_frame,cmd_len);
}

/* rx remote request stand frame to cmd str */
void rsff_to_cmd(CAN_RxHeaderTypeDef* header)
{
	RemoteStandFrame_def cmd_frame;
	uint32_t cmd_len;
	cmd_frame.cmd = CMD_RSFF;
	int2hex(header->StdId,3,cmd_frame.id);
	int2hex(header->DLC,1,&cmd_frame.len);
	cmd_len = 6;
	cmd_frame.cr = CMD_CR;
	cmd_response(&cmd_frame,cmd_len);
}

/* rx remote request extend frame to cmd str */
void reff_to_cmd(CAN_RxHeaderTypeDef* header)
{
	RemoteExtendFrame_def cmd_frame;
	uint32_t cmd_len;
	cmd_frame.cmd = CMD_REFF;
	int2hex(header->ExtId,8,cmd_frame.id);
	int2hex(header->DLC,1,&cmd_frame.len);
	cmd_len = 11;
	cmd_frame.cr = CMD_CR;
	cmd_response(&cmd_frame,cmd_len);
}

/* can recv interrupt callback */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_RxHeaderTypeDef can_rx_header;
	uint8_t can_rx_buf[8];
	int8_t can_rx_count = HAL_CAN_GetRxFifoFillLevel(hcan,CAN_RX_FIFO0);
	for(;can_rx_count > 0;can_rx_count--)
	{
		if (HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&can_rx_header,can_rx_buf) == HAL_OK)
		{ 
			if(can_rx_header.IDE == CAN_ID_STD)
			{
				if(can_rx_header.RTR == CAN_RTR_DATA)
				{
					sff_to_cmd(&can_rx_header,can_rx_buf);
				}
				else if(can_rx_header.RTR == CAN_RTR_REMOTE)
				{
					rsff_to_cmd(&can_rx_header);
				}
			}
			else if(can_rx_header.IDE == CAN_ID_EXT)
			{
				if(can_rx_header.RTR == CAN_RTR_DATA)
				{
					eff_to_cmd(&can_rx_header,can_rx_buf);
				}
				else if(can_rx_header.RTR == CAN_RTR_REMOTE)
				{
					reff_to_cmd(&can_rx_header);
				}
			}
		} 
	}
}

/* set can bitrate */
/* only change the hcan.Init.Prescaler value = 6 = 1Mbps*/
/* This command is only active if the CAN channel is closed */
void cmd_set_bitrate(char *cmd){
	uint8_t res_code=CMD_CR;
	SetupBitrate_def *p = (SetupBitrate_def*)cmd;
	if (hcan.State == HAL_CAN_STATE_READY){
		hcan.Init.Prescaler = bitrate_map[hex2int(&(p->speed),1)];
		if (HAL_CAN_Init(&hcan) != HAL_OK)
		{
			res_code = CMD_BELL;
		}
	}else{
		res_code = CMD_BELL;
	}
	cmd_response(&res_code,1);
}

/* return the version */
void cmd_version(char *cmd){
	char res_code[6]={CMD_VER,};
	res_code[1] = HW_VERSION[0];
	res_code[2] = HW_VERSION[1];
	res_code[3] = SW_VERSION[0];
	res_code[4] = SW_VERSION[1];
	res_code[5] = CMD_CR;
	cmd_response(res_code,6);
}

/* return the serial number */
void cmd_sn(char *cmd){
	char res_code[6]={CMD_SN,};
	/* Read MCU Id, 32-bit access */
	uint32_t MCU_Id = DBGMCU->IDCODE;
	int2hex(MCU_Id,4,res_code + 1);
	res_code[5] = CMD_CR;
	cmd_response(res_code,6);
}

/* set btr , not used!! */
void cmd_set_btr(char *cmd){
	uint8_t btr0=0,btr1=0;
	btr0 = hex2int(((SetupBtr_def*)cmd)->btr0,2);
	btr1 = hex2int(((SetupBtr_def*)cmd)->btr1,2);
	cmd_response(CMD_ACK,1);
	UNUSED(btr0);
	UNUSED(btr1);
}

/* open can */
void cmd_open(){
	uint8_t res_code=CMD_CR;
	if (HAL_CAN_Start(&hcan) != HAL_OK){
		res_code = CMD_BELL;
	}
	cmd_response(&res_code,1);
}

/* close can */
void cmd_close(){
	uint8_t res_code=CMD_CR;
	if (HAL_CAN_Stop(&hcan) != HAL_OK){
		res_code = CMD_BELL;
	}
	cmd_response(&res_code,1);
}

/* timestamp set, not used!! */
void cmd_timestamp(char *cmd){
	uint8_t onoff = hex2int(&((TimeStamp_def *)cmd)->timestamp,1);
	cmd_response(CMD_ACK,1);
	UNUSED(onoff);
}

/* tx stand can frame*/
void cmd_sff(char *cmd){
	uint8_t can_data[8]={0};
	uint8_t i;
	uint32_t mailbox;
	uint8_t res_code[2];
	CAN_TxHeaderTypeDef can_tx_header;
	StandFrame_def *p = (StandFrame_def*)cmd;
	can_tx_header.StdId = hex2int(p->id,3);
	can_tx_header.DLC = hex2int(&p->len,1);
	can_tx_header.IDE = CAN_ID_STD;
	can_tx_header.RTR = CAN_RTR_DATA;
	for(i=0;i<can_tx_header.DLC;i++){
		can_data[i] = hex2int(p->data+(i<<1),2);
	}
	if(HAL_CAN_AddTxMessage(&hcan,&can_tx_header,can_data,&mailbox) == HAL_OK){
		cmd_response("z\r",2);
	}
	else{
		res_code[0]=CMD_BELL;
		cmd_response(res_code,1);
	}
}

/* tx extend can frame*/
void cmd_eff(char *cmd){
	uint8_t can_data[8]={0};
	uint8_t i;
	uint32_t mailbox;
	uint8_t res_code[2];
	CAN_TxHeaderTypeDef can_tx_header;
	ExtendFrame_def *p = (ExtendFrame_def*)cmd;
	can_tx_header.ExtId = hex2int(p->id,8);
	can_tx_header.DLC = hex2int(&p->len,1);
	can_tx_header.IDE = CAN_ID_EXT;
	can_tx_header.RTR = CAN_RTR_DATA;
	for(i=0;i<can_tx_header.DLC;i++){
		can_data[i] = hex2int(p->data+(i<<1),2);
	}
	if(HAL_CAN_AddTxMessage(&hcan,&can_tx_header,can_data,&mailbox) == HAL_OK){
		cmd_response("z\r",2);
	}
	else{
		res_code[0]=CMD_BELL;
		cmd_response(res_code,1);
	}
}

/* tx remote request stand can frame*/
void cmd_rsff(char *cmd){
	uint8_t can_data[8]={0};
	uint32_t mailbox;
	uint8_t res_code[2];
	CAN_TxHeaderTypeDef can_tx_header;
	RemoteStandFrame_def *p = (RemoteStandFrame_def*)cmd;
	can_tx_header.StdId = hex2int(p->id,3);
	can_tx_header.DLC = hex2int(&p->len,1);
	can_tx_header.IDE = CAN_ID_STD;
	can_tx_header.RTR = CAN_RTR_REMOTE;
	if(HAL_CAN_AddTxMessage(&hcan,&can_tx_header,can_data,&mailbox) == HAL_OK){
		cmd_response("z\r",2);
	}
	else{
		res_code[0]=CMD_BELL;
		cmd_response(res_code,1);
	}
}

/* tx remote request extend can frame*/
void cmd_reff(char *cmd){
	uint8_t can_data[8]={0};
	uint32_t mailbox;
	uint8_t res_code[2];
	CAN_TxHeaderTypeDef can_tx_header;
	RemoteExtendFrame_def *p = (RemoteExtendFrame_def*)cmd;
	can_tx_header.ExtId = hex2int(p->id,8);
	can_tx_header.DLC = hex2int(&p->len,1);
	can_tx_header.IDE = CAN_ID_EXT;
	can_tx_header.RTR = CAN_RTR_REMOTE;
	if(HAL_CAN_AddTxMessage(&hcan,&can_tx_header,can_data,&mailbox) == HAL_OK){
		cmd_response("z\r",2);
	}
	else{
		res_code[0]=CMD_BELL;
		cmd_response(res_code,1);
	}
}

/* process one slcan cmd */
int slcan_cmd_task(char *cmd){
	//printf("cmd:%s\n",cmd);
	switch(cmd[0])
	{
		case CMD_SET_BITRATE:
			cmd_set_bitrate(cmd);
			break;
		case CMD_SET_BTR:
			cmd_set_btr(cmd);
			break;
		case CMD_OPEN:
			cmd_open();
			break;
		case CMD_CLOSE:
			cmd_close();
			break;
		case CMD_SFF:
			cmd_sff(cmd);
			break;
		case CMD_EFF:
			cmd_eff(cmd);
			break;
		case CMD_RSFF:
			cmd_rsff(cmd);
			break;
		case CMD_REFF:
			cmd_reff(cmd);
			break;
		case CMD_RSF:
			break;
		case CMD_TIMESTAMP:
			cmd_timestamp(cmd);
			break;
		case CMD_ACCEPT_MASK:
			break;
		case CMD_ACCEPT_VALUE:
			break;
		case CMD_VER:
			cmd_version(cmd);
			break;
		case CMD_SN:
			cmd_sn(cmd);
			break;
		default:
			break;

	}
	return 0;
}

/* get strings from usart and process  */
int slcan_cdc_process(char *st,int len){
	char cmd[50] ={0};
	int i = 0;
	for(i=0 ;i< len;i++){
		if (cmd_buf_off >= (CMD_BUF_MAX-1)){
			//printf("cmd_buff overflow\n");
			return -1;
		}
		//get a complete cmd str
		if(st[i] == CMD_CR && cmd_buf_off ){
			memcpy(cmd,cmd_buf,cmd_buf_off);
			cmd[cmd_buf_off] =0;
			cmd_buf_off = 0;
			//process it
			slcan_cmd_task(cmd);
		}else{
			cmd_buf[cmd_buf_off++] = st[i];
		}
	}
	return 0;
}

uint32_t hex2int(char *hex,uint8_t len) {
    uint32_t val = 0,i=0;
    while (i<len && hex[i] ) {
        // get current character then increment
        uint8_t byte = hex[i++]; 
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;   
		else return -1; 
        // shift 4 to make space for new digit, and add the 4 bits of the new digit 
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

void int2hex(uint32_t data,int len,char * out){
	while(len > 0){
		out[len-1] = hex_str_map[data & 0xf];
		data >>= 4;
		len--;
	}
}


void onlytest(char * uart_str){
	slcan_cdc_process(uart_str,strlen(uart_str));
}

/*
void main(int argc,char **argv){
	printf("slcan portocol test.\n");
	printf("----------------------\n");
	onlytest("-sfsfs\r-testtesttesttesttest");
	onlytest("\r123\rabcdefg\r");
	onlytest("\r123\rabcdefg\r");

	printf("%d\n",hex2int((char*)"021f2F",6));
	printf("%d\n",hex2int((char*)"0",1));

	onlytest("S0\rS1\rS2\rS3\rS4\rS5\rS6\rS7\rS8\r");

	onlytest("s1234\r");

	onlytest("O\rC\r");

	onlytest("t10021133\r");
	onlytest("T0000010021133\r");
	onlytest("T0000010021134\rZ0\rZ1\r");

	char t[10];
	int2hex(200,3,t);printf("%s\n",t);

}
*/

