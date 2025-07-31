
#include <stdint.h>


#define DEMCR        			*((volatile uint32_t*) 0xE000EDFCU )


#define ITM_STIMULUS_PORT0   	*((volatile uint32_t*) 0xE0000000 )
#define ITM_TRACE_EN          	*((volatile uint32_t*) 0xE0000E00 )


int __io_putchar(int ch)
{
	
	DEMCR |= (1 << 24);

	
	ITM_TRACE_EN |= ( 1 << 0);

	while(!(ITM_STIMULUS_PORT0 & 1));

	
	ITM_STIMULUS_PORT0 = ch;

	return 1;
}
