#include <l4/rmgr/rmgr.h>
#include <l4/rmgr/librmgr.h>
#include <l4/l4.h>
#include <l4io.h>

#include "hd.h"


#define IDE_PORT 0

#if IDE_PORT == 0   /* ide 0 */

#define IDE_HD_INTR                14
#define PORT_BASE               0x1F0


#else /* ide 1 */


#define IDE_HD_INTR                15
#define PORT_BASE               0x170

#endif /* IDE_PORT */


/*
 * PIC (8259) ports and codes
 */
#define	PIC1_OCW1		0x21
#define	PIC1_OCW2		0x20
#define	PIC1_OCW3		0x20

#define	PIC2_OCW1		0xA1
#define	PIC2_OCW2		0xA0
#define	PIC2_OCW3		0xA0
#define	PIC2_ISR_IRR		0xA0
#define	PIC2_IMR		0xA1

#define	READ_IRR		0xA
#define	READ_ISR		0xB
   
#define	SEOI			0x60
   
#define	SEOI_MASTER		(SEOI + 2)
#define	SEOI_HD 		(SEOI + IDE_HD_INTR - 8)
   
#define	SYS_PORT_A		0x92
   
/*
 * Hard disk controller ports, commands, and states
 */

#define	DATA_PORT		(PORT_BASE + 0)
#define	ERROR_PORT		(PORT_BASE + 1)
#define	SECTOR_COUNT_PORT	(PORT_BASE + 2)
#define	SECTOR_NO_PORT		(PORT_BASE + 3)
#define	CYLINDER_LOW_PORT	(PORT_BASE + 4)
#define	CYLINDER_HIGH_PORT	(PORT_BASE + 5)
#define	DRIVE_HEAD_SELECT_PORT	(PORT_BASE + 6)
#define	STATUS_PORT		(PORT_BASE + 7)
#define	COMMAND_PORT		(PORT_BASE + 7)

struct hd_stat {
    unsigned error_bit		:1;
    unsigned dummy1		:1;
    unsigned ecc_occurred_bit	:1;
    unsigned data_request_bit	:1;
    unsigned dummy2		:1;
    unsigned write_fault_bit	:1;
    unsigned drive_ready_bit	:1;
    unsigned busy_bit		:1;
};

struct hd_err {
    unsigned data_mark_ntf_bit	:1;
    unsigned track0_ntf_bit	:1;
    unsigned invalid_cmd_bit	:1;
    unsigned dummy1		:1;
    unsigned sector_id_ntf_bit	:1;
    unsigned dummy2		:1;
    unsigned data_error_bit	:1;
    unsigned bad_block_bit	:1;
};
          
#define	is_error(stat)	(stat->error_bit ||		\
			 stat->ecc_occurred_bit ||	\
			 stat->write_fault_bit ||	\
			 stat->drive_ready_bit == 0)
   
#define	DEVICE_NO_BIT		4
#define	LBA_MODE_BIT		6
   
#define	RECALIBRATE		0x10
#define	READ_SECTORS		0x20
#define	WRITE_SECTORS		0x30
#define	VERIFY_SECTORS		0x40
#define	IDENTIFY_DEVICE		0xEC
                                
#define	RETRIES_ENABLED		0
#define	RETRIES_DISABLED	1 
   
#define	STEP_RATE_35US		0000
   
#define	MULTIPLE_SECS_WORD	47
#define	CAPABILITIES_WORD	49
#define	SECTOR_CAPACITY		57

#define	LBA_SUPPORTED_BIT	9

#define DRIVE_NR                0


struct req0 {
    unsigned sector_no_field	:30;
    unsigned drive_no_field	:2;
};
       
struct req1 {
#define REQ_SEC_BITS		10
    unsigned sectors_field	:9;
    unsigned request_field	:1;
    unsigned buf_addr_field	:22;
};

#define	READ_OP			0
#define	WRITE_OP		1

#define	OPEN_REQUEST		0x01000008
#define	CLOSE_REQUEST		0xfffffff1

#define	OK_REPLY		0x01000000
#define	SECTOR_NO_OVRFLW_REPLY	0x80000001
#define TOO_MANY_SECTORS_REPLY	0x80000002
#define	ILLEGAL_REQUEST_REPLY	0x80000003

struct hd_vector hd[MAX_DRIVES];

int
reg_to_irq(int irq)
{
    int ret;
    dword_t dummy;
    l4_msgdope_t dummydope;

    ret = rmgr_get_irq(irq);
//    printf("idedrv: rmgr_get_irq() => %d\n", ret);

    if ( ret != 0 )
    {
	printf("rmgr does not like us - no int!\n");
	return -1;
    }

    ret = l4_ipc_receive(L4_INTERRUPT(irq), 0,
			 &dummy, &dummy, &dummy, 
			 L4_IPC_TIMEOUT_NULL,
			 &dummydope);

    if ( ret != 2 )
	panic("Could not register irq (%x, %x)\n", ret, L4_INTERRUPT(irq));
    
    return 0;
}


/*
 * HD routines
 */
void piceoi(void)
{
    cli();

    outb(PIC2_OCW2, SEOI_HD);
    outb(PIC2_OCW3, READ_ISR);
    if ( inb(PIC2_ISR_IRR) == 0 )
	outb(PIC1_OCW2, SEOI_MASTER);

    outb(PIC2_OCW1, inb(PIC2_OCW1) & ~(1 << (IDE_HD_INTR-8)));

    sti();
}

int
disk_status_and_drq(void)
{
    int status, i;
    struct hd_stat *sp = (struct hd_stat *) &status;

    for ( i = 50000; i; i-- )
    {
	status = inb(STATUS_PORT);
	if ( sp->busy_bit == 0 && sp->data_request_bit )
	    break;
    }

    if ( i == 0 )
	sp->busy_bit = sp->data_request_bit = 0;

    return status;
}

int
disk_status(void)
{
    int status, i;
    struct hd_stat *sp = (struct hd_stat *) &status;

    for ( i = 50000; i; i-- )
    {
	status = inb(STATUS_PORT);
	if ( sp->busy_bit == 0 )
	    break;
    }

    if (i == 0)
	sp->busy_bit = sp->data_request_bit = 0;

    return status ;
}

int
disk_error(void)
{     
     int error = inb(ERROR_PORT);

     return error;
}

void
wait_for_interrupt(int irq)
{
    dword_t dummy;
    l4_msgdope_t dummydope;
    int result;

    //printf("wait_for_irq %d", irq);
    result = l4_ipc_receive(L4_INTERRUPT(irq), 0, &dummy, &dummy,
			    &dummy, L4_IPC_NEVER, &dummydope);

    if ( result == 0 )
	piceoi();
    //enter_kdebug("got int");
}

void
switch_on_drive_lamp(void)
{
    outb(SYS_PORT_A, inb(SYS_PORT_A) | 0x40);
}

void
switch_off_drive_lamp(void)
{
     outb(SYS_PORT_A, inb(SYS_PORT_A) & ~0x40);
}


/*
 * Command to hard disk
 */
void
send_cmd_to_disk(int command, int drive_no, int sector_no, int sectors)
{
    outb(SECTOR_NO_PORT, sector_no);
    outb(CYLINDER_LOW_PORT, sector_no >> 8);
    outb(CYLINDER_HIGH_PORT, sector_no >> 16);
    outb(DRIVE_HEAD_SELECT_PORT, ((sector_no >> 24) & 0xf) +
	 ((drive_no & (MAX_DRIVES - 1)) << DEVICE_NO_BIT) +
	 (1 << LBA_MODE_BIT));
    outb(SECTOR_COUNT_PORT, sectors);
    outb(COMMAND_PORT, command);
}

/*
 * Open controller
 */
int
open_controller(void)
{
    if (reg_to_irq(IDE_HD_INTR)) return -1;
     
    /* Do we have a controller? */
    outb(SECTOR_COUNT_PORT, 1);
    if (inb(SECTOR_COUNT_PORT) != 1) return -1;
    outb(SECTOR_COUNT_PORT, 2);
    if (inb(SECTOR_COUNT_PORT) != 2) return -1;
    outb(SECTOR_NO_PORT, 3);
    if (inb(SECTOR_NO_PORT) != 3) return -1;
    outb(SECTOR_NO_PORT, 4);
    if (inb(SECTOR_NO_PORT) != 4) return -1;

    cli();
    outb(PIC2_OCW1, inb(PIC2_OCW1) & ~(1 << (IDE_HD_INTR - 8)));
    sti();

    return 0;
}     

/*
 * Open drive
 */
int
open_drive(int drive_no)
{
    int status, multiple_secs, capabilities = 0, sectors = 0, i, word;
    struct hd_stat *sp = (struct hd_stat *) &status;
 
    hd[drive_no].sectors_per_drive = 0;

    printf("idedrv: open drive\n");

    send_cmd_to_disk(IDENTIFY_DEVICE, drive_no, 0, 0);
    wait_for_interrupt(IDE_HD_INTR);
    status = disk_status();
    if ( sp->drive_ready_bit == 0 )
	return 0;

    disk_status_and_drq();
     
    for (i = 0; i < 256; i++)
    {
	word = inw(DATA_PORT);
	if ( i == MULTIPLE_SECS_WORD )
	    multiple_secs = word & 0xff;
	else if ( i == CAPABILITIES_WORD )
	    capabilities = word;
	else if ( i == SECTOR_CAPACITY )
	{
	    sectors = (inw(DATA_PORT) << 16) + word;
	    ++i;
	}
    }

    if ( capabilities & (1 << LBA_SUPPORTED_BIT) )
	return sectors;
    else
	return 0;
}     


unsigned int total_sectors_read = 0, total_sectors_written = 0;

/*
 * Read block
 */
int
read_block(int drive_no, int sector_no, int sectors, int *buffer_address)
{
    int status;
    struct hd_stat *sp = (struct hd_stat *)&status;

    switch_on_drive_lamp();
    send_cmd_to_disk(READ_SECTORS, drive_no, sector_no, sectors);
    do {
	wait_for_interrupt(IDE_HD_INTR);
	status = disk_status_and_drq();
	if (sp->data_request_bit)       
	    insw(DATA_PORT, buffer_address, SECTOR_SIZE / 2);
	if (is_error(sp)) break;            
	(char *)buffer_address += SECTOR_SIZE;
	--sectors; 
	total_sectors_read++;
    } while (sectors > 0);
    switch_off_drive_lamp();
    if (is_error(sp)) return status + (disk_error() << 8);
    else return OK_REPLY;
}

/*
 * Write block
 */
int
write_block(int drive_no, int sector_no, int sectors, int *buffer_address)
{
    int status;
    struct hd_stat *sp = (struct hd_stat *)&status;

    switch_on_drive_lamp();
    send_cmd_to_disk(WRITE_SECTORS, drive_no, sector_no, sectors);
    while (1) {
	status = disk_status_and_drq();
	if (is_error(sp)) break;
	outsw(DATA_PORT, buffer_address, SECTOR_SIZE / 2);
	wait_for_interrupt(IDE_HD_INTR);
	(char *)buffer_address += SECTOR_SIZE;
	--sectors ; 
	total_sectors_written++;
	if (sectors > 0) continue;
	else { status = disk_status(); break; }
    }
    switch_off_drive_lamp();
    if (is_error (sp)) return status + (disk_error() << 8);
    else return OK_REPLY;     
}
