#ifndef __UART_DRIVER_H__
#define __UART_DRIVER_H__

#include "FreeRTOS.h"
#include "semphr.h"
#include "main.h"
#include "task.h"
#include "timers.h"
#include "usart.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <stdarg.h> 


void printf_usart3(char *format,...);
#endif
