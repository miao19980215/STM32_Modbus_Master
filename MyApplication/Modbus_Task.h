#ifndef __MODBUS_TASK_H__
#define __MODBUS_TASK_H__

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "event_groups.h"
#include "modbus_driver.h"

void MyModbusTask(void *argument);
#endif
