#include "uart_driver.h"
//===============printf函数重定义==============
static int inHandlerMode(void)
{
	return __get_IPSR();
}
void printf_usart3(char *format,...)
{
	char buf[64];
	if(inHandlerMode() !=0)
	{
		taskDISABLE_INTERRUPTS();
	}
	else
	{
		while(HAL_UART_GetState(&huart3) == HAL_UART_STATE_BUSY_TX)
		{
			taskYIELD();
		}
	}
	va_list ap;
	va_start(ap, format);
	if(vsprintf(buf, format, ap)>0)
	{
		HAL_UART_Transmit(&huart3,(uint8_t *)buf,strlen(buf),500);
	}
	va_end(ap);
	if(inHandlerMode() !=0)
	{
		taskENABLE_INTERRUPTS();
	}
}

int fputc(int ch,FILE *f)
{
	HAL_UART_Transmit(&huart3,(uint8_t *)&ch,1,HAL_MAX_DELAY);
	return ch;
}
