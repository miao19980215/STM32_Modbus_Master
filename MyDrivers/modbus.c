#include "modbus.h"
#include "uart_driver.h"


#define lowByte(w) ((w) & 0xff)
#define highByte(w) ((w) >> 8)
modbus_Handler_t ModbusH;
static void vTimerCallbackT35(TimerHandle_t *pxTimer);
static void vTimerCallbackTimeout(TimerHandle_t *pxTimer);
const unsigned char fctsupported[] =
{
    MB_FC_READ_COILS,
    MB_FC_READ_DISCRETE_INPUT,
    MB_FC_READ_REGISTERS,
    MB_FC_READ_INPUT_REGISTER,
    MB_FC_WRITE_COIL,
    MB_FC_WRITE_REGISTER,
    MB_FC_WRITE_MULTIPLE_COILS,
    MB_FC_WRITE_MULTIPLE_REGISTERS
};
void ModbusInit(modbus_Handler_t * modH)
{
	modH->QueueModbusRxHandle = xQueueCreate(3, sizeof(modbus_RxData_t));
	xTaskCreate(StartTaskModbusMaster,"StartTaskModbusMaster",256, modH,osPriorityAboveNormal, &(modH->myTaskModbusAHandle));
	modH->xTimerTimeout=xTimerCreate("xTimerTimeout",  
				pdMS_TO_TICKS(modH->u16timeOut),     		
				pdFALSE,         
				( void * )modH->xTimerTimeout,    
				(TimerCallbackFunction_t) vTimerCallbackTimeout);
	modH->QueueTelegramHandle = xQueueCreate(MAX_TELEGRAMS, sizeof(modbus_t));					
	modH->xTimerT35 = xTimerCreate("TimerT35",
				pdMS_TO_TICKS(5),pdFALSE, 
				( void * )modH->xTimerT35,
				(TimerCallbackFunction_t) vTimerCallbackT35);
}

void vTimerCallbackT35(TimerHandle_t *pxTimer)
{
	if( (TimerHandle_t *)ModbusH.xTimerT35 ==  pxTimer )
	{
		xTimerStop(ModbusH.xTimerTimeout,0);
		xTaskNotify(ModbusH.myTaskModbusAHandle, 0, eSetValueWithOverwrite);
	}
}
void vTimerCallbackTimeout(TimerHandle_t *pxTimer)
{
	if( (TimerHandle_t *)ModbusH.xTimerTimeout ==  pxTimer )
	{
		xTaskNotify(ModbusH.myTaskModbusAHandle, NO_REPLY, eSetValueWithOverwrite);
	}
}


void ModbusQuery(modbus_Handler_t * modH, modbus_t telegram )
{
	xQueueSendToBack(modH->QueueTelegramHandle, &telegram, 0);
}

void StartTaskModbusMaster(void *argument)
{
  modbus_Handler_t *modH =  (modbus_Handler_t *)argument;
  uint32_t ulNotificationValue;
  modbus_t telegram;
  for(;;)
  {
	  xQueueReceive(modH->QueueTelegramHandle, &telegram, portMAX_DELAY);
	  SendQueryToSlave(modH, telegram);
	  ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);	  
	  modH->lastError = 0;
      if(ulNotificationValue == NO_REPLY)
      {
    	  modH->mod_state = Com_Idle;
    	  modH->lastError = NO_REPLY;
    	  modH->u16errCnt++;
		  printf_usart3("modbus rx fail  \r\n");	
    	  continue;
      }
	  modH->RxQueueNum = uxQueueMessagesWaiting(modH->QueueModbusRxHandle);
	  xQueueReceive(modH->QueueModbusRxHandle, &modH->modbus_RxData,0);
	  for (int i = 0; i < modH->modbus_RxData.RxDataLen; i++)
		{
			printf_usart3("%x ", modH->modbus_RxData.RxData[i]);
		}
       printf_usart3("modbus rx \r\n");			
	  xTimerStop(modH->xTimerTimeout,0); // cancel timeout timer
	  uint8_t u8exception = validateAnswer(modH);
	  if (u8exception != 0)
	  {
		 modH->mod_state = Com_Idle;
	     continue;
	  }
//	  printf_usart3("RS485 3  \r\n");
//	  if(modH->modbus_RxData.RxData[0]==0x04)
//	  {
//		xQueueSend(Queue_Rx485Message,&modH->modbus_RxData ,0);	  
//	  }
//	  if(modH->modbus_RxData.RxData[0]==0x05)
//	  {
//		  xQueueSend(Queue_Rx485Sound,&modH->modbus_RxData ,0);
//		printf_usart3("RS485 5  \r\n");
//	  }
//	  if(modH->modbus_RxData.RxData[0]==0x01)
//	  {
//		 xQueueSend(Queue_Rx485Led,&modH->modbus_RxData ,0);
//		printf_usart3("RS485 1  \r\n");
//	  }
//	  xQueueSend(Queue_Rx485Message,&modH->modbus_RxData ,0);
	  modH->lastError = u8exception;
	  modH->mod_state = Com_Idle;
	  continue;
	 }
}
int8_t SendQueryToSlave(modbus_Handler_t *modH,modbus_t telegram )
{
	uint8_t u8regsno, u8bytesno;
	int8_t  error = 0;
	if (modH->mod_state != Com_Idle) error = Err_Polling ;
	if ((telegram.slave_id==0) || (telegram.slave_id>247)) error = Err_Bad_Slave_ID;
	if(error)
	{
		 modH->lastError = error;
		 return error;
	}
	modH->au16regs = telegram.au16reg;
	modH->TxBuffer[ ID ]         = telegram.slave_id;
	modH->TxBuffer[ FUNC ]       = telegram.modbus_fct;
	modH->TxBuffer[ ADD_HI ]     = highByte(telegram.u16RegAdd );
	modH->TxBuffer[ ADD_LO ]     = lowByte( telegram.u16RegAdd );
	switch(telegram.modbus_fct)
	{
		case MB_FC_READ_COILS:
		case MB_FC_READ_DISCRETE_INPUT:
		case MB_FC_READ_REGISTERS:
		case MB_FC_READ_INPUT_REGISTER:
			modH->TxBuffer[ NB_HI ]      = highByte(telegram.u16CoilsNo );
			modH->TxBuffer[ NB_LO ]      = lowByte( telegram.u16CoilsNo );
			modH->TxBufferSize = 6;
			break;
		case MB_FC_WRITE_COIL:
			modH->TxBuffer[ NB_HI ]      = ((modH->au16regs[0] > 0) ? 0xff : 0);
			modH->TxBuffer[ NB_LO ]      = 0;
			modH->TxBufferSize = 6;
			break;
		case MB_FC_WRITE_REGISTER:
			modH->TxBuffer[ NB_HI ]      = highByte(modH->au16regs[0]);
			modH->TxBuffer[ NB_LO ]      = lowByte(modH->au16regs[0]);
			modH->TxBufferSize = 6;
			break;
		case MB_FC_WRITE_MULTIPLE_COILS: 
			u8regsno = telegram.u16CoilsNo / 16;
			u8bytesno = u8regsno * 2;
			if ((telegram.u16CoilsNo % 16) != 0)
			{
				u8bytesno++;
				u8regsno++;
			}
			modH->TxBuffer[ NB_HI ]      = highByte(telegram.u16CoilsNo );
			modH->TxBuffer[ NB_LO ]      = lowByte( telegram.u16CoilsNo );
			modH->TxBuffer[ BYTE_CNT ]    = u8bytesno;
			modH->TxBufferSize = 7;
			for (uint16_t i = 0; i < u8bytesno; i++)
			{
				if(i%2)
				{
					modH->TxBuffer[ modH->TxBufferSize ] = lowByte( modH->au16regs[ i/2 ] );
				}
				else
				{
					 modH->TxBuffer[  modH->TxBufferSize ] = highByte(  modH->au16regs[ i/2] );
				}
				modH->TxBufferSize++;
			}
			break;
		case MB_FC_WRITE_MULTIPLE_REGISTERS:
			modH->TxBuffer[ NB_HI ]      = highByte(telegram.u16CoilsNo );
			modH->TxBuffer[ NB_LO ]      = lowByte( telegram.u16CoilsNo );
			modH->TxBuffer[ BYTE_CNT ]    = (uint8_t) ( telegram.u16CoilsNo * 2 );
			modH->TxBufferSize = 7;

			for (uint16_t i=0; i< telegram.u16CoilsNo; i++)
			{
				modH->TxBuffer[  modH->TxBufferSize ] = highByte(  modH->au16regs[ i ] );
				modH->TxBufferSize++;
				modH->TxBuffer[  modH->TxBufferSize ] = lowByte(  modH->au16regs[ i ] );
				modH->TxBufferSize++;
			}
			break;
	}
	sendTxBuffer(modH);
	modH->mod_state = Com_Waiting;
	modH->lastError = 0;
	return 0;
}
void sendTxBuffer(modbus_Handler_t *modH)
{
    uint16_t u16crc = calcCRC(modH->TxBuffer, modH->TxBufferSize);
    modH->TxBuffer[modH->TxBufferSize] = u16crc >> 8;
    modH->TxBufferSize++;
    modH->TxBuffer[ modH->TxBufferSize ] = u16crc & 0x00ff;
    modH->TxBufferSize++;
    if (modH->EN_Port != NULL)
    {
    	HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_SET);
    }
	HAL_UART_Transmit_DMA(modH->port, modH->TxBuffer, modH->TxBufferSize);	
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
     if (modH->EN_Port != NULL)
     {
    	 while((modH->port->Instance->SR & USART_SR_TC) ==0 )
    	 {
    		taskYIELD();
    	 }
    	 HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET);
     }
     xQueueGenericReset(modH->QueueModbusRxHandle, pdFALSE);
     modH->TxBufferSize = 0;
	 xTimerReset(modH->xTimerTimeout,0);
     modH->u16OutCnt++;
}
uint16_t calcCRC(uint8_t *Buffer, uint8_t u8length)
{
    unsigned int temp, temp2, flag;
    temp = 0xFFFF;
    for (unsigned char i = 0; i < u8length; i++)
    {
        temp = temp ^ Buffer[i];
        for (unsigned char j = 1; j <= 8; j++)
        {
            flag = temp & 0x0001;
            temp >>=1;
            if (flag)
                temp ^= 0xA001;
        }
    }
    // Reverse byte order.
    temp2 = temp >> 8;
    temp = (temp << 8) | temp2;
    temp &= 0xFFFF;
    return temp;
}
int16_t validateAnswer(modbus_Handler_t *modH)
{
    uint16_t u16MsgCRC =
        ((modH->modbus_RxData.RxData[modH->modbus_RxData.RxDataLen - 2] << 8)
         | modH->modbus_RxData.RxData[modH->modbus_RxData.RxDataLen - 1]); 
    if (calcCRC(modH->modbus_RxData.RxData,modH->modbus_RxData.RxDataLen-2) != u16MsgCRC)
    {
    	modH->u16errCnt ++;
        return NO_REPLY;
    }
    if ((modH->modbus_RxData.RxData[ FUNC ] & 0x80) != 0)
    {
    	modH->u16errCnt ++;
        return Err_EXCEPTION ;
    }
    bool isSupported = false;
    for (uint8_t i = 0; i< sizeof( fctsupported ); i++)
    {
        if (fctsupported[i] == modH->modbus_RxData.RxData[FUNC])
        {
            isSupported = 1;
            break;
        }
    }
    if (!isSupported)
    {
    	modH->u16errCnt ++;
        return EXC_FUNC_CODE;
    }
    return 0; 
}
