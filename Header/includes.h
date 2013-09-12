
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_dac.h"
#include "stm32f10x_tim.h"

#include "fifo.h"
#include "misc.h"
#include "uart.h"
#include "main.h"
#include "console.h"
#include "atomic.h"
#include "sheppard.h"

