
/*

todo:
re-check atomic access with both 16 & 32 bit, using different clock speeds and alignments

char = 1
short = 2
int = 4
long = 4
structs padded! :-( 16bit members are aligned to 4byte boundary
Little Endian

	Main project
	Read/Write 32bit access is atomic, including indirect access
	char 1 byte
	int 4 bytes
	long 4 bytes


Thread mode	(from reset)	Privilege and Stack selected by CONTROL register,
				reset default is Privileged and Main stack

Handler mode	(during ISR)	Always privileged, always Main stack

Unprivileged	can't use SCB, System Timer, or NVIC

				can't use MSR MRS CPS instructions to access:
				IPSR, EPSR, IEPSR, IAPSR, EAPSR, PSR, MSP,
				PSP, PRIMASK, BASEPRI, BASEPRI_MAX, FAULTMASK, or CONTROL

				can use MSR MRS to access:
				APSR

Privileged	can do everything.

http://armcryptolib.das-labor.org/trac/wiki

*/

#include "..\Header\includes.h"

//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

//********************************************************************************************************
// Local defines
//********************************************************************************************************

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Private variables
//********************************************************************************************************

	static volatile uint8_t	tickflag;

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

//********************************************************************************************************
// Public functions
//********************************************************************************************************

void SysTick_Handler(void)
{
	tickflag=TRUE;
}

void main_fly(void)
{
}

int main(void)
{
	GPIO_InitTypeDef	gpio_init;
	int	tempint;

	__disable_irq();
	SCB->VTOR = 0x08004000;
	__enable_irq();

	uart1_init(38400, 16, main_fly, NULL);

	//Enable the two led outputs for debugging
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	gpio_init.GPIO_Pin   = GPIO_Pin_8 + GPIO_Pin_9;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio_init);

	console_init();
	sheppard_init();

	//10mS system tick
	SysTick_Config(SystemCoreClock * 0.01); //system_stm32f10x.c

	while(1)
	{
		while(!tickflag);
		tickflag=FALSE;
		sheppard_timer();
	};
	/*
	while(1)
    {
		uart1_fifo_rx_ptr = &console_fifo_tx;
		console_device = CONSOLE_DEVICE_UART1;
		while(1)
			console_main();
    };
*/
	return 0;
}

//********************************************************************************************************
// Private functions
//********************************************************************************************************
