#ifndef __MODBUS_DRIVER_H__
#define __MODBUS_DRIVER_H__

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "uart_driver.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "event_groups.h"

void ModbusInitPrivately(void);
void USART2_Rx485_Init_Privately(void);
void USART2_Rx485_IDLE(UART_HandleTypeDef *huart);
#endif
