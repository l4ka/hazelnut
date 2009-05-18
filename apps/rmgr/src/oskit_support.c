#include <stdio.h>
#include <unistd.h>

#include <flux/machine/pc/direct_cons.h>
#include <flux/machine/pc/reset.h>
#include <flux/machine/pio.h>
#include <flux/c/string.h>
#include <flux/base_critical.h>

#include <l4/l4.h>

extern int hercules;
extern int kernel_running;

static void herc_direct_cons_putchar(unsigned char c);
static void kern_putchar(unsigned char c);
int have_hercules(void);
void my_pc_reset(void);

int
putchar(int c)
{
  (kernel_running)
    ? kern_putchar(c)
    : (hercules)
      ? herc_direct_cons_putchar(c)
      : direct_cons_putchar(c);
  
  return c;
}

int
getchar(void)
{
  return direct_cons_getchar();
}

void 
my_pc_reset(void)
{
  outb_p(0x70, 0x80);
  inb_p (0x71);

  while (inb(0x64) & 0x02)
    ;

  outb_p(0x70, 0x8F);
  outb_p(0x71, 0x00);

  outb_p(0x64, 0xFE);

  for (;;)
    ;
}

void 
_exit(int fd)
{
  char c;

  if (kernel_running)
    {
      printf("\nReturn reboots, \"k\" enters L4 kernel debugger...\n");
      c = getchar();
  if (c == 'k' || c == 'K') 
    enter_kdebug("before reboot");
    }
  else
    {
      printf("\nReturn reboots...\n");
      getchar();
    }

  my_pc_reset();
}

/* modified direct_cons_putchar from oskit/libkern */
#define HERC_VIDBASE ((unsigned char *)0xb0000)
void
herc_direct_cons_putchar(unsigned char c)
{
  static int ofs = -1;

  base_critical_enter();

  if (ofs < 0)
    {
      /* Called for the first time - initialize.  */
      ofs = 0;
      herc_direct_cons_putchar('\n');
    }

  switch (c)
    {
    case '\n':
      bcopy(HERC_VIDBASE+80*2, HERC_VIDBASE, 80*2*24);
      bzero(HERC_VIDBASE+80*2*24, 80*2);
      /* fall through... */
    case '\r':
      ofs = 0;
      break;

    case '\t':
      ofs = (ofs + 8) & ~7;
      break;

    default:
      /* Wrap if we reach the end of a line.  */
      if (ofs >= 80) {
	herc_direct_cons_putchar('\n');
      }

      /* Stuff the character into the video buffer. */
      {
	volatile unsigned char *p = HERC_VIDBASE
	  + 80*2*24 + ofs*2;
	p[0] = c;
	p[1] = 0x0f;
	ofs++;
      }
      break;
    }

  base_critical_leave();
}

static void
kern_putchar(unsigned char c)
{
  outchar(c);
  if (c == '\n')
    outchar('\r');
}

int
have_hercules(void)
{
  unsigned short *p, p_save;
  int count = 0;
  unsigned long delay;
  
  /* check for video memory */
  p = (unsigned short*)0xb0000;
  p_save = *p;
  *p = 0xaa55; if (*p == 0xaa55) count++;
  *p = 0x55aa; if (*p == 0x55aa) count++;
  *p = p_save;
  if (count != 2)
    return 0;

  /* check for I/O ports (HW cursor) */
  outb_p(0x3b4, 0x0f);
  outb	(0x3b5, 0x66);
  for (delay=0; delay<0x100000UL; delay++)
    asm volatile ("nop" : :);
  if (inb_p(0x3b5) != 0x66)
    return 0;
  
  outb_p(0x3b4, 0x0f);
  outb	(0x3b5, 0x99);
  for (delay=0; delay<0x100000UL; delay++)
    asm volatile ("nop" : :);
  if (inb_p(0x3b5) != 0x99)
    return 0;

  /* reset position */
  outb_p(0x3b4, 0x0f);
  outb	(0x3b5, 0x0);

  return 1;
}
