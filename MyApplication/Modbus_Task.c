#include "Modbus_Task.h"
#include "modbus_driver.h"
#include "Modbus.h"

void MyModbusTask(void *argument)
{
	modbus_t telegram;
    telegram.slave_id = 4;				 
    telegram.modbus_fct = 03; 		 	
    telegram.u16RegAdd = 0x5; 		
    telegram.u16CoilsNo = 1; 		
//    telegram.au16reg = ModbusDATA; 	
    for(;;)
    {
		ModbusQuery(&ModbusH, telegram); 
		printf_usart3("modbus tx  \r\n");	
		vTaskDelay(pdMS_TO_TICKS(800));
    }
}
