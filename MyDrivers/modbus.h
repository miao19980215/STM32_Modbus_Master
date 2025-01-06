#ifndef MODBUS_H_
#define MODBUS_H_

#include "usart.h"
#include <stdbool.h>
#include "gpio.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#define MAX_BUFFER 64
#define MAX_TELEGRAMS 3 
#define RXDATAMAXLEN 20
typedef struct
{
    uint8_t slave_id;          
    uint8_t modbus_fct;         
    uint16_t u16RegAdd;    		
    uint16_t u16CoilsNo;   		
    uint16_t *au16reg;     		
}
modbus_t;
typedef struct
{
	uint8_t RxData[RXDATAMAXLEN];
	uint8_t RxDataLen;
}modbus_RxData_t;
typedef struct
{
	UART_HandleTypeDef *port; 
	GPIO_TypeDef* EN_Port; 
	uint16_t EN_Pin;  
	int16_t lastError;
	uint8_t TxBuffer[MAX_BUFFER]; 
	uint8_t TxBufferSize;
	uint16_t *au16regs;
	uint16_t u16InCnt, u16OutCnt, u16errCnt; 
	uint16_t u16timeOut;
	uint8_t RxQueueNum;
	int8_t mod_state;
	modbus_RxData_t  modbus_RxData;
	QueueHandle_t QueueModbusRxHandle;
	QueueHandle_t QueueTelegramHandle;
	TaskHandle_t myTaskModbusAHandle;
	xTimerHandle xTimerT35;
	xTimerHandle xTimerTimeout;
}
modbus_Handler_t;
enum Err_List
{
    Err_Polling                   = -2,
    Err_BUFF_OVERFLOW             = -3,
    Err_BAD_CRC                   = -4,
    Err_EXCEPTION                 = -5,
	Err_BAD_SIZE                  = -6,
	Err_BAD_ADDRESS               = -7,
	Err_TIME_OUT				  = -8,
	Err_Bad_Slave_ID			  = -9

};
enum Com_States
{
    Com_Idle                     = 0,
    Com_Waiting                  = 1

};
enum MESSAGE
{
    ID                             = 0, 
    FUNC, 
    ADD_HI, 
    ADD_LO, 
    NB_HI, 
    NB_LO, 
    BYTE_CNT  
};
enum MB_FC
{
    MB_FC_NONE                     = 0, 
    MB_FC_READ_COILS               = 1,	
    MB_FC_READ_DISCRETE_INPUT      = 2,	
    MB_FC_READ_REGISTERS           = 3,	
    MB_FC_READ_INPUT_REGISTER      = 4,	
    MB_FC_WRITE_COIL               = 5,	
    MB_FC_WRITE_REGISTER           = 6,	
    MB_FC_WRITE_MULTIPLE_COILS     = 15,	
    MB_FC_WRITE_MULTIPLE_REGISTERS = 16	
};
enum
{
    NO_REPLY = 255,
    EXC_FUNC_CODE = 1,
    EXC_ADDR_RANGE = 2,
    EXC_REGS_QUANT = 3,
    EXC_EXECUTE = 4
};
extern modbus_Handler_t ModbusH;
void ModbusInit(modbus_Handler_t * modH);
void StartTaskModbusMaster(void *argument);
int8_t SendQueryToSlave(modbus_Handler_t *modH,modbus_t telegram );
void ModbusQuery(modbus_Handler_t * modH, modbus_t telegram );
void sendTxBuffer(modbus_Handler_t *modH);
uint16_t calcCRC(uint8_t *Buffer, uint8_t u8length);
int16_t validateAnswer(modbus_Handler_t *modH);
#endif
