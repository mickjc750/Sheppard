/*
Console module, 
*/

	#include "..\Header\includes.h"

//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

	#define INPUT_FIFO_SIZE 16

// fail waiting for input after this much time
// TIMER_TICK must be defined (in seconds) as interval between calls to console_timer()
	#define INPUT_PROMPT_TIME	60

	#define	BOOT_SECTION		0x00000000

	#define TEXTOUT_P(arg1)		fifo_write_string(&uart1_fifo_tx, arg1)
	#define TEXTOUT(arg1)		fifo_write_string(&uart1_fifo_tx, arg1)
	#define TEXTOUT_CHAR(arg1)	fifo_write_char(&uart1_fifo_tx, arg1)
	#define TEXTOUT_INT(arg1)	fifo_write_string(&uart1_fifo_tx, ascii_long((long)arg1, text, 10, 0, 0, 0))
	#define TEXTOUT_HEX(arg1)	fifo_write_string(&uart1_fifo_tx, ascii_ulong((long)arg1, text, 16, 8, '0'))

//********************************************************************************************************
// Local defines
//********************************************************************************************************

	//Compatibility for AVR to ARM port
	#define	PSTR(arg)	arg

	enum input_result_enum
	{
		INPUT_RESULT_OK,
		INPUT_RESULT_TIMEOUT,
		INPUT_RESULT_ABORTED,
		INPUT_RESULT_NIL
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************

	struct fifo_struct console_fifo_tx;
	char console_echo=TRUE;
	unsigned char console_device = CONSOLE_DEVICE_OTHER;

//********************************************************************************************************
// Private variables
//********************************************************************************************************

	static volatile int time;
	static char text[10];

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

	static void bootload(void);

	static enum input_result_enum getint(int *target, int low, int high);
	static enum input_result_enum getline(char* buffer, int maxlen);
	static enum input_result_enum getyn(char* target);
	static enum input_result_enum getkey(char* target, char low, char high);

	//special Boot Load functions after optimization PRAGMA
	static void boot_jump(void);
	static void BootJump(uint32_t firmwareStartAddress);

//********************************************************************************************************
// Public functions
//********************************************************************************************************

void console_timer(void)
{
	if(time)
		time--;
}

void console_init(void)
{
	fifo_init(&console_fifo_tx, INPUT_FIFO_SIZE, NULL, NULL, NULL);
}

void console_main(void)
{
	enum input_result_enum success;
	int option, tempint1, tempint2;

	char tempchar, finished=FALSE;
	do
	{
		TEXTOUT_P(PSTR("\r\n\
\r\n\
****** Main  Menu ******\r\n\
\r\n\
1. Upload new firmware\r\n\
2. Speed set\r\n\
3. Envelope set\r\n\
4. Enter\r\n\
5. Exit\r\n\
6. Finalize\r\n\
7. Tonal\r\n\
8. Minor\r\n\
9. Major\r\n\
0. exit\r\n\
"));

		success=getint(&option, 0, 9);

		//if input failed, exit menu, or refresh menu
		if(success == INPUT_RESULT_TIMEOUT || success == INPUT_RESULT_ABORTED)
			finished=TRUE;
		else if(success == INPUT_RESULT_OK)
		{
			switch(option)
			{
				case 1:
					bootload();
					break;
				case 2:
					getint(&option, -40000, +40000);
					sheppard_speedset(option);
					break;
				case 3:
					TEXTOUT_P(PSTR("Center "));
					getint(&tempint1, 0, 255);
					TEXTOUT_P(PSTR("Width "));
					getint(&tempint2, 1, 200);
					sheppard_envelopeset(tempint1, tempint2);
					break;
				case 4:
					sheppard_enter();
					break;
				case 5:
					sheppard_exit();
					break;
				case 6:
					sheppard_finalize();
					break;
				case 7:
					sheppard_styleset(STYLE_TONE);
					break;
				case 8:
					sheppard_styleset(STYLE_MINOR);
					break;
				case 9:
					sheppard_styleset(STYLE_MAJOR);
					break;
			};
		};//INPUT_RESULT_NIL will simply refresh

	}while(!finished);
	TEXTOUT_P(PSTR("Exit\r\n"));
}


//********************************************************************************************************
// Private functions
//********************************************************************************************************

static void bootload(void)
{
	if( console_device == CONSOLE_DEVICE_OTHER )
	{
		TEXTOUT_P(PSTR("Cannot use Boot Loader from this I/O device (hit space)\r\n"));
		getkey(NULL, ' ', '~');
	}
	else
	{
		__disable_irq();

		//Bootloader will detect which uart is left enabled.
		if( console_device != CONSOLE_DEVICE_UART1 )
				USART_DeInit(USART1);
		if( console_device != CONSOLE_DEVICE_UART2 )
				USART_DeInit(USART2);
		if( console_device != CONSOLE_DEVICE_UART3 )
				USART_DeInit(USART3);

		boot_jump();
	};
}

// get an integer from the user and write it to *target, returns OK/ABORTED/TIMEOUT
// aborts if ESC is pressed.
// note that any input that atoi cannot recognise as a number, will result in 0.
static enum input_result_enum getint(int* target, int low, int high)
{
	enum input_result_enum success;
	char finished=FALSE;
	int value;

	do
	{
		TEXTOUT_P(PSTR(">"));
		success=getline(text, 9);
		if(success==INPUT_RESULT_OK)
		{
			if(strlen(text))
			{
				value=atoi(text);
				//out of range
				if( value < low || high < value )
				{
					TEXTOUT_P(PSTR("Out of range! Enter number between "));
					TEXTOUT_INT(low);
					TEXTOUT_P(PSTR(" and "));
					TEXTOUT_INT(high);
					TEXTOUT_P(PSTR("\r\n"));
				}	
				//ok
				else
				{
					finished=TRUE;
					*target=value;
				};
			}
			else
			{
				success=INPUT_RESULT_NIL;
				finished=TRUE;
			};
		}
		else
			finished=TRUE;
	}while(!finished);

	return success;
}

// get's a line from input, terminates at CR, ignores outside 0x20-0x7E, writes it to *buffer
// returns OK/ABORT/TIMEOUT, does NOT return INPUT_RESULT_NIL, as an empty line is a valid entry.
// abort only occurs if ESC is hit, in which case buffer is terminated string of length 0
// a line input length of 0 is a valid entry (returns OK)
// maxlen includes terminator, must be non0
// attempting to enter text longer than maxlen-1 will simply return a truncated string once CR is received
// if echo is enabled chars will not be echo'd beyond maxlen-1
static enum input_result_enum getline(char* buffer, int maxlen)
{
	enum input_result_enum success;
	int index=0;		//indexes terminator in buffer
	char tempchar;
	char finished=FALSE;

	buffer[index]=0;
	do
	{		
		//wait for input or timeout
		time=(INPUT_PROMPT_TIME/TIMER_TICK);

		while( time && (console_fifo_tx.empty == TRUE) )
			main_fly();

		if(console_fifo_tx.empty == TRUE)
		{
			buffer[0]=0;
			success=INPUT_RESULT_TIMEOUT;
			finished=TRUE;
		}
		else
		{
			tempchar=fifo_read_char(&console_fifo_tx);
			
			//abort (esc)
			if(tempchar==0x1B)
			{
				buffer[0]=0;
				success=INPUT_RESULT_ABORTED;
				finished=TRUE;
			}
			//backspace
			else if(tempchar==0x08 && index)
			{
				if(console_echo)
					TEXTOUT_P(PSTR("\x08 \x08"));
				buffer[--index]=0;
			}
			//text
			else if(0x1F < tempchar && tempchar < 0x7F)
			{
				if(index<maxlen-1)
				{
					if(console_echo)
						TEXTOUT_CHAR(tempchar);
					buffer[index++]=tempchar;
					buffer[index]=0;
				};
			}
			//CR
			else if(tempchar=='\r')
			{
				finished=TRUE;
				success=INPUT_RESULT_OK;
			};
		};
	} while(!finished);

	if(console_echo)
	{
		TEXTOUT_P(PSTR("\r\n"));
	};
	return success;
}

// get's a y or n response, writes non-0 to target if yes
// returns OK/ABORT/TIMEOUT
// abort only occurs if ESC is hit, in which case target is not modified
static enum input_result_enum getyn(char* target)
{
	enum input_result_enum success;
	char tempchar;
	char finished=FALSE;

	do
	{
		//wait for input or timeout
		time=INPUT_PROMPT_TIME/TIMER_TICK;
		while(time && (console_fifo_tx.empty == TRUE))
			main_fly();
		
		if(console_fifo_tx.empty == TRUE)
		{
			success=INPUT_RESULT_TIMEOUT;
			finished=TRUE;
		}
		else
		{
			tempchar=fifo_read_char(&console_fifo_tx);
			if(tempchar=='y' || tempchar=='Y')
			{
				if(console_echo)
					TEXTOUT_CHAR(tempchar);
				*target=TRUE;
				finished=TRUE;
				success=INPUT_RESULT_OK;
			}
			else if(tempchar=='n' || tempchar=='N')
			{
				if(console_echo)
					TEXTOUT_CHAR(tempchar);
				*target=FALSE;
				finished=TRUE;
				success=INPUT_RESULT_OK;
			}
			else if(tempchar==0x1B)
			{
				finished=TRUE;
				success=INPUT_RESULT_ABORTED;
			};
		};
		
	}while(!finished);

	TEXTOUT_P(PSTR("\r\n"));

	return success;
}

// get's a single character within range
// returns OK/ABORT/TIMEOUT
// abort only occurs if ESC is hit, in which case target is not modified
static enum input_result_enum getkey(char* target, char low, char high)
{
	enum input_result_enum success;
	char tempchar;
	char finished=FALSE;
	
	do
	{
		//wait for input or timeout
		time=INPUT_PROMPT_TIME/TIMER_TICK;
		while(time && (console_fifo_tx.empty == TRUE))
			main_fly();
		
		if(console_fifo_tx.empty == TRUE)
		{
			success=INPUT_RESULT_TIMEOUT;
			finished=TRUE;
		}
		else
		{
			tempchar=fifo_read_char(&console_fifo_tx);
			if(low <= tempchar && tempchar <= high)
			{
				if(console_echo)
					TEXTOUT_CHAR(tempchar);
				if(target)
					*target=tempchar;
				finished=TRUE;
				success=INPUT_RESULT_OK;
			}
			else if(tempchar==0x1B)
			{
				finished=TRUE;
				success=INPUT_RESULT_ABORTED;
			};
		};
		
	}while(!finished);

	TEXTOUT_P(PSTR("\r\n"));

	return success;
}

#pragma GCC optimize ("0")
static void boot_jump(void)
{
	BootJump(BOOT_SECTION);
}

static void BootJump(uint32_t firmwareStartAddress)
{
	__ASM volatile ("LDR SP, [R0]");	//          ;Load new stack pointer address
	__ASM volatile ("LDR PC, [R0, #4]");//        	;Load new program counter address
}
