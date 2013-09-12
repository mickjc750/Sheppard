/*
Interface to console module
*/

//********************************************************************************************************
// Public defines
//********************************************************************************************************

	enum
	{
		CONSOLE_DEVICE_OTHER,
		CONSOLE_DEVICE_UART1,
		CONSOLE_DEVICE_UART2,
		CONSOLE_DEVICE_UART3
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************

	extern struct fifo_struct	console_fifo_tx;
	extern char 				console_echo;
	extern unsigned char		console_device;

//********************************************************************************************************
// Public prototypes
//********************************************************************************************************

	void console_init(void);
	void console_main(void);
	void console_timer(void);
	void console_test(void);
	
