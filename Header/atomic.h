
//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Public defines
//********************************************************************************************************

	static __INLINE uint32_t atomic_enter(void)
	{
		uint32_t retval;
		retval = __get_PRIMASK();
		__disable_irq();
		return retval;
	}

	static __INLINE uint32_t atomic_exit(uint32_t temp)
	{
		__set_PRIMASK(temp);
		return 0;
	}

	//Wraps a block of code with ISR save/disable/restore using a for loop
	#define ATOMIC_BLOCK for( uint32_t atomic_temp = atomic_enter(), atomic_flag = 1; atomic_flag; atomic_flag = atomic_exit(atomic_temp) )

//********************************************************************************************************
// Public prototypes
//********************************************************************************************************

