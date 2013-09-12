
//********************************************************************************************************
// Public variables
//********************************************************************************************************

	extern char atomic_flag;

//********************************************************************************************************
// Public defines
//********************************************************************************************************

	static __INLINE void atomic_disable(void)
	{
		while(atomic_flag);	//hang forever if app attempts to nest atomic blocks
		__disable_irq();
		atomic_flag=TRUE;
	}

	static __INLINE void atomic_enable(void)
	{
		atomic_flag=FALSE;
		__enable_irq();
	}

	//Wraps a block of code with ISR save/disable/restore using a for loop
	#define ATOMIC_BLOCK for(atomic_disable(); atomic_flag ; atomic_enable())

//********************************************************************************************************
// Public prototypes
//********************************************************************************************************

