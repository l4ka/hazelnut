#ifndef __L4_SERVER_RMGR_H__
#define __L4_SERVER_RMGR_H__

#include <l4/rmgr/server_proto.h>

#define RMGR_LTHREAD_PAGER (0)
#define RMGR_LTHREAD_SUPER (1)

#define RMGR_RMGR (0)		/* PROTOCOL: rmgr meta protocol */
#define RMGR_RMGR_PING (0xf3)	/* ping -- returns ~arg in d2 */

#define RMGR_RMGR_MSG(action, arg) L4_PROTO_MSG(RMGR_RMGR, (action), (arg))

#define RMGR_MEM (1)		/* PROTOCOL: memory operations */
#define RMGR_MEM_FREE (1)	/* free physical page */
#define RMGR_MEM_FREE_FP (2)	/* free an fpage */
#define RMGR_MEM_INFO (3)	/* dump info about memory regions */
#define RMGR_MEM_RESERVE (4)	/* reserve chunk of free pages */
#define RMGR_MEM_GET_PAGE0 (5)	/* explicit deliver physical page 0 */

#define RMGR_MEM_MSG(action) L4_PROTO_MSG(RMGR_MEM, (action), 0)

#define RMGR_TASK (2)		/* PROTOCOL: task operations */
#define RMGR_TASK_ALLOC (1)	/* allocate task number */
#define RMGR_TASK_GET (2)	/* allocate specific task number */
#define RMGR_TASK_FREE (3)	/* free task number */
#define RMGR_TASK_CREATE (4)	/* create a task XXX */
#define RMGR_TASK_DELETE (5)	/* delete a task */
#define RMGR_TASK_SET_SMALL (6)	/* set a task's small address space number */
#define RMGR_TASK_SET_PRIO (7)	/* set a task's priority */
#define RMGR_TASK_GET_ID (8)	/* get task id by module name */

#define RMGR_TASK_CREATE_WITH_PRIO (9)	/* create a task and set priority XXX */
#define RMGR_TASK_SET_ID (10)	/* get task id by module name */

#define RMGR_TASK_MSG(action, taskno) \
  L4_PROTO_MSG(RMGR_TASK, (action), (taskno))

#define RMGR_IRQ (3)
#define RMGR_IRQ_GET (2)	/* allocate an interrupt number */
#define RMGR_IRQ_FREE (3)	/* free an interrupt number */

#define RMGR_IRQ_MSG(action, intno) \
  L4_PROTO_MSG(RMGR_IRQ, (action), (intno))

#define RMGR_SYNC (4)
#define RMGR_WAIT_EVENT              (1)
#define RMGR_SIGNAL_EVENT            (2)
#define RMGR_ENTER_CRITICAL_SECTION  (3)
#define RMGR_LEAVE_CRITICAL_SECTION  (4)

#define RMGR_SYNC_MSG(action, ident) \
  L4_PROTO_MSG(RMGR_SYNC, (action), (ident))

#define RMGR_MEM_RES_FLAGS_MASK 0x0FC0
#define RMGR_MEM_RES_DMA_ABLE   0x0040	/* memory has to lay below 16 MB */
#define RMGR_MEM_RES_UPWARDS    0x0080	/* search upwards */
#define RMGR_MEM_RES_DOWNWARDS  0x0000	/* search downwards (default) */

#endif

