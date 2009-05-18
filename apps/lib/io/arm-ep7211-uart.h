
#if defined(NATIVE)
#define HWBASE		0x80000000
#else
#define HWBASE		0xFFF00000
#endif

#define HWBASE1		(HWBASE + 0x00000000)
#define HWBASE2		(HWBASE + 0x00001000)

#define DATA(x)		(*(volatile byte_t*) (((x)+0x480)))
#define STATUS(x)	(*(volatile dword_t*) (((x)+0x140)))

#define UTXFF		(1 << 23)
#define URXFE		(1 << 22)
