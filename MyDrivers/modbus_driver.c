#include "modbus_driver.h"
#include "Modbus.h"
#include "queue.h"
extern DMA_HandleTypeDef hdma_usart2_rx; 
QueueHandle_t    Queue_Rx485Message = NULL;   //数据接收队列
QueueHandle_t    Queue_Rx485Led = NULL;   //数据接收队列
QueueHandle_t    Queue_Rx485Sound = NULL;   //数据接收队列
modbus_RxData_t   Rx485l_Data ;
void ModbusInitPrivately(void)
{
	ModbusH.port =  &huart2;
	ModbusH.u16timeOut = 1000;
	ModbusH.EN_Port = NULL;
	ModbusH.EN_Port = RS485_RE_GPIO_Port;
	ModbusH.EN_Pin = RS485_RE_Pin;
	ModbusInit(&ModbusH);
//	Queue_Rx485Message = xQueueCreate(1, sizeof(Rx485l_Data));
//	Queue_Rx485Led = xQueueCreate(1, sizeof(Rx485l_Data));
//	Queue_Rx485Sound = xQueueCreate(1, sizeof(Rx485l_Data));
//	if (Queue_Rx485Message == NULL || Queue_Rx485Led==NULL || Queue_Rx485Sound==NULL)
//	{
//		printf_usart3("Failed to create Queue_Rx485Message!\n");
//	}
	printf_usart3("RS485 Init  \r\n");	
}
void USART2_Rx485_Init_Privately(void)
{ 
	__HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);   
}
BaseType_t    xHigherPriorityTaskWoken; 
void USART2_Rx485_IDLE(UART_HandleTypeDef *huart)
{
	uint32_t temp=0;
    if((__HAL_UART_GET_FLAG(huart,UART_FLAG_IDLE) != RESET))  
    {
		if (ModbusH.port == &huart2 )
		{
			__HAL_UART_CLEAR_IDLEFLAG(&huart2);
			HAL_UART_DMAStop(&huart2);
			temp = __HAL_DMA_GET_COUNTER(&hdma_usart2_rx); 
			ModbusH.modbus_RxData.RxDataLen =  RXDATAMAXLEN- temp; 
			xQueueSendToBackFromISR(ModbusH.QueueModbusRxHandle,&ModbusH.modbus_RxData, &xHigherPriorityTaskWoken);
			__HAL_DMA_SET_COUNTER(&hdma_usart2_rx,RXDATAMAXLEN);
			__HAL_DMA_ENABLE(&hdma_usart2_rx);				
			xTimerResetFromISR(ModbusH.xTimerT35, &xHigherPriorityTaskWoken);	
			HAL_UART_Receive_DMA(&huart2,ModbusH.modbus_RxData.RxData,ModbusH.modbus_RxData.RxDataLen);
		}
		
    }  
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
