/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips-pr31700.c
 * Description:   Kdebug glue for the MIPS PR31700.
 *                
 * @LICENSE@
 *                
 * $Id: mips-pr31700.c,v 1.2 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mips/pr31700/platform.h>

#define UART_TX		(1 << 14)
#define UART_ENABLE	1

#if CONFIG_DEBUGGER_COMSPEED == 57600
# define UART_SPEED 4
#else
# error Wrong speed for serial line
#endif

#undef UART_SPEED
#define UART_SPEED 23

void kdebug_init_arch()
{
    siu_out(UARTA_CONTROL_2, UART_SPEED);
}

void kdebug_hwreset()
{

}

void putc(char c)
{
    dword_t tmp = 0x44444444;
    siu_out(UARTA_CONTROL_1, UART_ENABLE | UART_TX);

    siu_out(UARTA_DMA_CONTROL_1, (dword_t)virt_to_phys(&tmp));
    siu_out(UARTA_DMA_CONTROL_2, 3);
}

char getc()
{
    return 0;
}


