/*********************************************************************
 *		  
 * Copyright (C) 2002,	Karlsruhe University
 *		  
 * File path:	  x86/io_mapping.c
 * Description:	  Implementation of mapping database for IO fpages
 *		  
 * @LICENSE@
 *		  
 * $Id: io_mapping.c,v 1.2 2002/05/21 15:39:29 stoess Exp $
 *		  
 ********************************************************************/

#include <universe.h>

#if defined(CONFIG_IO_FLEXPAGES)

#include <mapping.h>
#include <x86/memory.h>
#include <x86/io_mapping.h>
#include <x86/space.h>

#if defined(CONFIG_AUTO_IO_MAP)
#define IOPBM_DEFAULT_VALUE (0)
#else
#define IOPBM_DEFAULT_VALUE (~0)
#endif

#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a<b?b:a)

#define IOFP_PORT(x) ((dword_t) (((x) >> 12) & 0x0000FFFF))
#define IOFP_LD_SIZE(x) ((dword_t) (((x) >>  2) & 0x0000003F))
#define IOFP_SIZE(x) ((dword_t) (1 << IOFP_LD_SIZE(x)))

#define IOMDB_LO_MASK	     0xFFFF0000
#define IOMDB_TID_MASK	     0x000000FF

#define IOMDB_HI_MASK	     0xFFFF8000
#define IOMDB_DUMMY_MASK     0x00004000
#define IOMDB_DEPTH_MASK     0x00003FFF
#define IOMDB_SUP_DEPTH	     0x00004000

#define FLUSH_OWN_SPACE (1<<31)

// #define IO_MDB_DEBUG 1

/* The Kernel Page Directory */
static space_t *kspace;

/* The virtual address of the default iopbm */
static dword_t iopbm_virt;

/* The physical address of the default iopbm */
static ptr_t iopbm_phys;

/* The Pagetable containing the entry to the IOPM */
static pgent_t *iopbm_pgt;

/* The TSS */
extern x86_tss_t __tss;



/*  IO Mapping Node */
class io_mnode_t
{
public:
    dword_t w1;			      /* Lower	Bound (16), TaskID (8) */
    dword_t w2;			      /* Higher Bound (17), Dummy Marker (1),
					 Tree Depth (14) */
    io_mnode_t *next, *prev;	      /* Next/Prev Node in Mapping Tree */
    io_mnode_t *next_st, *prev_st;    /* Next/Prev Node in Tasks List */

    io_mnode_t *parent;		     /* Parent */

    dword_t get_lo(void)
	{ return (w1 >> 16); }

    void set_lo(dword_t l)
	{ w1 = (w1 & IOMDB_TID_MASK) | (l << 16); }
	
    l4_threadid_t get_tid(void)
	{ return (l4_threadid_t) { raw : { ( (w1 & IOMDB_TID_MASK) << L4_X0_THREADID_BITS + L4_X0_VERSION_BITS)}};}

    void set_tid(l4_threadid_t t)
	{ w1 = (w1 & IOMDB_LO_MASK) | ( (t.raw >> L4_X0_THREADID_BITS + L4_X0_VERSION_BITS ) & IOMDB_TID_MASK );}

    dword_t get_hi(void)
	{ return (w2 >> 15); }

    void set_hi(dword_t h)
	{ w2 = (w2 & (IOMDB_DEPTH_MASK | IOMDB_DUMMY_MASK) ) |	(h << 15); }

		
    dword_t get_dummy(void)
	{ return (w2 & IOMDB_DUMMY_MASK); }

    void set_dummy(void)
	{ w2 |= IOMDB_DUMMY_MASK; }

    dword_t get_depth(void)
	{ return (w2 & IOMDB_DEPTH_MASK); }

    void set_depth(dword_t s)
	{ w2 = (w2 & (IOMDB_HI_MASK | IOMDB_DUMMY_MASK)) |  (s & IOMDB_DEPTH_MASK) ; }

    void set_values(dword_t lo, dword_t hi, l4_threadid_t tid, dword_t depth, byte_t dummy){
	w1 = (lo << 16) | ((tid.raw >> L4_X0_THREADID_BITS + L4_X0_VERSION_BITS) & IOMDB_TID_MASK);
	w2 = ((hi << 15) | (dummy ? IOMDB_DUMMY_MASK : 0)) | (depth & IOMDB_DEPTH_MASK);
    }
    
    void enq_thread_list(io_mnode_t *before){
	this->next_st = before;
	this->prev_st = before->prev_st;
	before->prev_st = this;
	this->prev_st->next_st = this;
    }

    void deq_thread_list(){
	this->next_st->prev_st = this->prev_st;
	this->prev_st->next_st = this->next_st;
	
    }

    void enq_subtree(io_mnode_t *after){
	this->next = after->next;
	this->prev = after;
	this->next->prev = this;
	after->next = this;
    }
    
    void deq_subtree(){
	this->next->prev = this->prev;
	this->prev->next = this->next;
    }

};
		


/* The Initial Sigma0 node */
static io_mnode_t sigma0_io_space;

/* The IO MDB Terminator */
static io_mnode_t io_mdb_end;


/* 
 * Lock mechanism:
 *	Implemented by a binary semaphore
 *	TODO: This could be crap
 */


/* The Semaphore */
static dword_t io_mdb_lock;

#if 0
static void io_mdb_p(void){
    __asm__ __volatile__ (
	"1:	     \n\t"
	"lock;"
	"btsl $0, %0 \n\t"
	"jc 1b	     \n\t"
	:
	:"m"(io_mdb_lock));
	
}



static void io_mdb_v(void){
    __asm__ __volatile__ (
	"lock;"
	"btrl $0, %0 \n\t"
	:
	:"m"(io_mdb_lock));
}
#else
#define io_mdb_p()
#define io_mdb_v()
#endif 


/* 
 * int get_iopbm()
 * 
 * returns the first valid node of a task 
 */

static inline io_mnode_t *get_iospace(tcb_t *task){
	
    dword_t *pgd = task->space->pagedir();
    return ((io_mnode_t *) pgd[IO_SPACE_IDX])->next_st;
}

/* 
 * int get_iopbm()
 * 
 * Sets the PGT entry to IO-Space
 */

static inline void set_iospace(tcb_t *task, io_mnode_t *node){
	
    dword_t *pgd = task->space->pagedir();
    pgd[IO_SPACE_IDX] = (dword_t) node;
}

/* 
 * int get_iopbm()
 * 
 * returns the physical address of the iopbm of a task 
 */

static inline ptr_t get_iopbm(space_t *space){

    return (ptr_t) space->pgent(pgdir_idx(iopbm_virt))->subtree(space, 0)->\
	next(space, pgtab_idx(iopbm_virt), 0)->address(space, 0);

}
	
/* 
 * int install_iopbm()
 * 
 * installs an intialised 8k Region as IOPBM 
 *
 */


static int install_iopbm(tcb_t *task, dword_t new_iopbm){

	
    /* 
     * Allocate a Pagetable.
     * Do not use ->make_subtree, as we have to 
     * allocate and set up the pagetable _before_
     * installing it
     */
    pgent_t *pgt_new =	(pgent_t *) kmem_alloc(PAGE_SIZE);
    
    if (pgt_new == NULL){
	panic("install_iopbm(): out of memory\n");
    }
    
    /*	Copy all entries from Kernel Page Table	 */
    pgent_t *k	    = iopbm_pgt;
    pgent_t *n	    = pgt_new;
    
    for (int i=0; i<1024; i++){
	n->copy(task->space, 1, k, kspace, 1);
	k = k->next(kspace, 1, 1); 
	n = n->next(task->space, 1, 1);
    } 
    
    
    /*	Set the two special entries  */
    pgt_new->next(task->space, pgtab_idx(iopbm_virt), 0)->\
	set_entry(task->space, virt_to_phys(new_iopbm), 0, 0);
    pgt_new->next(task->space, pgtab_idx(iopbm_virt), 0)->\
	set_flags(task->space, 0, PAGE_VALID | PAGE_WRITABLE);
    
    pgt_new->next(task->space, pgtab_idx(iopbm_virt + 0x1000), 0)->\
	set_entry(task->space, virt_to_phys( new_iopbm + 0x1000), 0, 0); 
    pgt_new->next(task->space, pgtab_idx(iopbm_virt + 0x1000), 0)->\
	set_flags(task->space, 0, PAGE_VALID | PAGE_WRITABLE); 

	
    /* Install new Pagetable (writable) */
    task->space->pgent(pgdir_idx(iopbm_virt))->\
	set_entry(task->space, (dword_t) virt_to_phys( (ptr_t) pgt_new), 1, 0); 


    /* Set Flags */
    task->space->pgent(pgdir_idx(iopbm_virt))->
	set_flags(task->space, 0, PAGE_VALID | PAGE_WRITABLE);
	
    if (task == get_current_tcb()){
	flush_tlbent((ptr_t)iopbm_virt); 
	flush_tlbent((ptr_t)(iopbm_virt + 0x1000));
    }

    return 0;
	

}

/* 
 * void release_iopbm()
 * 
 * releases the IOPBM of a task and sets the pointers back to the  default-IOPBM
 *
 */
 
static void release_iopbm(tcb_t *task){

	
    /*	Get the special pagetable  */
    ptr_t iopbm = get_iopbm(task->space);

    if (iopbm == iopbm_phys)
	return;
	
    iopbm = phys_to_virt(iopbm);

    pgent_t *pgt = task->space->pgent(pgdir_idx(iopbm_virt))->subtree(task->space, 0);

    /*	Insert the default PGT	*/
    task->space->pgent(pgdir_idx(iopbm_virt))->\
	set_entry(task->space, (dword_t) virt_to_phys( (ptr_t) iopbm_pgt), 1, 0);

    /* Set Flags */
    task->space->pgent(pgdir_idx(iopbm_virt))->\
	set_flags(task->space,0, PAGE_VALID | PAGE_WRITABLE);
	
    /*	Release the Pagetable  */
    kmem_free((ptr_t) pgt, PAGE_SIZE);
	
    /* Release the IOPBM */
    kmem_free(iopbm, X86_IOPERMBITMAP_SIZE / 8);

	
    /* Flush the corresponding TLB entries */
    if (task == get_current_tcb()){
	flush_tlbent((ptr_t)iopbm_virt); 
	flush_tlbent((ptr_t)(iopbm_virt + 0x1000));
    }


}

/* 
 * int zero_iopbm()
 * 
 * returns  0 on success
 *	   -1 on error
 */

static int zero_iopbm(tcb_t *task, dword_t port, dword_t size)
{
    dword_t count;
       
    // printf("zero_iopbm(): port=%x, size=%x, tid=%x\n", port, size, task->myself.raw);

    ptr_t iopbm = get_iopbm(task->space);

#if !defined(CONFIG_AUTO_IO_MAP)
  
    /* 
     * If the task had no mappings before, it has the default IOPBM (which is set to ~0 in case 
     * of no implicit mapping). Create a new one, set it to 0 between port and port + size, 
     * and to ~0 else 
     */

    if (iopbm == iopbm_phys){
	
	    
	ptr_t new_iopbm, tmp;

	new_iopbm = kmem_alloc(X86_IOPERMBITMAP_SIZE / 8);
	    
	if (new_iopbm == NULL){
	    panic("zero_iopbm(): out of memory\n");
	}

	if (install_iopbm(task, (dword_t) new_iopbm)){
	    printf("zero_iopbm(): install_iopbm failed\n");
	    return -1;
	}	
	    
	tmp = new_iopbm;
	/* Set the bits in our new IOPBM */

	/* Set the first set of bits to ~0, until we reach the dword of the port */
	    
	count = port >> 5; 
	    
	for (dword_t i=0; i < count; i++)
	    *(new_iopbm++) = ~0; 
	    

	/* Zero the first dword, in which the port has its bit */	    
	count = MIN(size , 32 - (port & 0x1F) );
	*(new_iopbm++) =  ~(( (dword_t)~0 >> (32 - count)) << (port & 0x1F)); 
	    
	size -= count;
	    
	/* Zero dwords between first and last dword */
	while( size >= 32){
	    *(new_iopbm++) = 0;
	    size -= 32;
	}
	    
	/* Zero the last dword, in which port+size has its bits */
	if (size){
	    *(new_iopbm++) = ~( (dword_t)~0 >> (32 - size));
	}
	    
	/* The remainder of the bits is to set to 1 */
	while(new_iopbm < tmp + 2048){
	    *(new_iopbm++) = ~0;
	}

	return 0;

    } /* if (iopbm == iopbm_phys) */
    else
#endif /* !defined(CONFIG_AUTO_IO_MAP) */
	iopbm = phys_to_virt(iopbm);
    
    /* Jump to the right location */
    iopbm += (port >> 5);

    /* Zero the first dword, in which the port has its bit */	    
    count = MIN(size , 32 - (port & 0x1F) );

    *(iopbm++) &=  ~(( (dword_t)~0 >> (32 - count)) << (port & 0x1F)); 

    size -= count;

    /* Zero dwords between first and last dword */
    while( size >= 32){
	*(iopbm++) = 0;
	size -= 32;
    }
	    
    /* Zero the last dword, in which port+size has its bits */
    if (size){
	*(iopbm++) &= ~((dword_t)~0 >> (32 - size));
    }

    return 0;
}

/* 
 * int set_iopbm()
 * 
 * returns  0 on success
 *	   -1 on error
 */

static int set_iopbm(tcb_t *task, dword_t port, dword_t size){

    int count;
    ptr_t iopbm;
    
    iopbm = get_iopbm(task->space);

    // printf("set_iopbm(): port=%x, size=%x, tid=%x\n", port, size, task->myself.raw);
 
#if defined(CONFIG_AUTO_IO_MAP)

    /*
     * Check if mapper has the default iopbm which is set to 0 everywhere in case of 
     * implicit mapping enabled.
     */
    
    if (iopbm == iopbm_phys){

	iopbm = kmem_alloc(X86_IOPERMBITMAP_SIZE / 8);
	
	if (iopbm == NULL){
	    panic("set_iopbm(): out of memory\n");
	}

	if (install_iopbm(task, (dword_t) iopbm)){
	    // printf("set_iopbm(): install_iopbm failed\n");
	    return -1;
	}

    }	 
    else
#endif
	iopbm = phys_to_virt(iopbm);
    /* Set the bits in the IOPBM */

    iopbm += (port >> 5);
	
    /* Set the first dword, in which the port has its bit */	    
    
    count = MIN(size , 32 - (port & 0x1F) );
    *(iopbm++) |=   (((dword_t)~0 >> (32 - count)) << (port & 0x1F)); 
	    
    size -= count;
	    
    /* Set dwords between first and last dword */
    while( size >= 32){
	*(iopbm++) = ~0;
	size -= 32;
    }
	
    /* Set the last dword, in which port+size has its bits */
    if (size){
	*(iopbm++) |= ( (dword_t)~0 >> (32 - size));
    }

    return 0;
    
}

		
#if defined(IO_MDB_DEBUG)
void dump_io_mdb(void){
    io_mnode_t *n = &sigma0_io_space;
    while (n){
	printf("io_mnode: lo=%x, hi=%x, tid=%x, depth=%x, addr=%x, parent=%x\n",\
	       n->get_lo(), n->get_hi(), n->get_tid(), n->get_depth(), n, n->parent);
	n = n->next;
    }
		       
}	
#else 
void dump_io_mdb(void){}
#endif

/* 
 * int unmap_subtree()
 * 
 * modifies (and removes if needed) the subtree below in order to unmap the
 * given region out of the  subtree
 * 
 */

static int unmap_subtree(io_mnode_t *root, dword_t lo, dword_t hi, dword_t mapmask){
	
    tcb_t *flushee;
    io_mnode_t *n;
	
    io_mnode_t *n_left_end;
    dword_t split_depth;
	
    /* 
     * split_depth marks the tree depth of an eventual split, set it to infinite 
     * in case of splitting the nodes, the depth is the upper bound for splitting
     */
    split_depth = IOMDB_SUP_DEPTH;
    n_left_end = NULL;
	
    mapmask &= FLUSH_OWN_SPACE;

    n = mapmask ? root : root->next;
	
    /* Modify/Remove Subtree */
    while (n->get_depth() > root->get_depth() || mapmask){
	    
	// printf("n_first->node: %x-%x depth=%x, tid=%x\n", n->get_lo(),
	// n->get_hi(), n->get_depth(), n->get_tid());
	
	
	/* Set mapmask to 0 as we have to remove ourselves only once */
	mapmask = 0;
	flushee = tid_to_tcb(n->get_tid());

	/* Check what we have to do: */
	if (lo <= n->get_lo()){
		
	    if (hi >= n->get_hi()){
			
		// printf("unmap_subtree(): begins before, ends after-> free\n");
		/*  We can free the node to flush, as its region is a subset of the io_fp we want to flush  */
			
		/* Chain out of task list */
		n->deq_thread_list();
					
		/* Chain out of subtree */
		n->deq_subtree();

		if (set_iopbm(flushee, n->get_lo(), n->get_hi() - n->get_lo())){
		    printf("unmap_subtree(): set_iopbm failed\n");
		    return -1;
		}
			
		/* If the node is not part of a split subtree, set split mark to infinite */
		if (n->get_depth() <= split_depth){
				
		    split_depth = IOMDB_SUP_DEPTH;
		}
			
		/* Free Node */
		{
		    io_mnode_t *tmp = n->prev;
		    mdb_free_buffer((ptr_t) n, sizeof(io_mnode_t));					      
		    n = tmp;
		}
	    }
		
	    else{
		/* io_fp begins before node to flush and ends in or before it */
		printf("unmap_subtree(): begins before, ends in or before\n");
			
		if (hi > n->get_lo()){
				
		    /* io_fp begins before and ends in it, set the IOPBM and resize the node */
		    if (set_iopbm(flushee, n->get_lo(), hi - n->get_lo())){
			printf("unmap_subtree(): set_iopbm failed\n");
			return -1;
		    }
				
		    n->set_lo(hi);
		}
			
		/* If the node is not part of a split subtree, set split mark to infinite */
		if (n->get_depth() <= split_depth){
				
		    split_depth = IOMDB_SUP_DEPTH;
		}

	    }
	
	}
	    
	else{
	    /* (lo > n->get_lo()) */
		    
	    if (hi > n->get_hi()){

		/* io_fp begins in or after node to flush and ends after it */
		// printf("unmap_subtree(): begins in or after, ends after\n");
			
		if (lo < n->get_hi()){

		    /* io_fp begins in node to flush: resize it */
		    if (set_iopbm(flushee, lo, n->get_hi() - lo)){
			printf("unmap_subtree(): set_iopbm failed\n");
			return -1;
		    }
		    n->set_hi(lo);
			
		}
				
		/* If we are in a split subtree, we have to place this node on the left subtree */
		if (n->get_depth() > split_depth){
				
		    /* Store next node for loop */
		    io_mnode_t *n_tmp = n->next;
				
		    /* Chain out of tree */
		    n->deq_subtree();

		    /* Chain in tree */
		    n->enq_subtree(n_left_end);
				
		    /* New end of left subtree */
		    n_left_end = n;
				
		    /* Restore node for loop */
		    n = n_tmp->next->prev;
		}
			
		else{
		    split_depth = IOMDB_SUP_DEPTH;
		}

	    }
		    
	    else{
		
		/* 
		 * io_fp begins and ends in the node to flush - this is the more
		 * complicated part
		 * 
		 * - split subtree
		 * - allocate a new node 
		 * - chain in new node an store it
		 * - take care about splitting 'til we reach the end of this
		 *   subtree
		 * 
		 */
		// printf("unmap_subtree(): begins in, ends in ->split\n");

		if (set_iopbm(flushee, lo, hi - lo)){
		    printf("unmap_subtree(): set_iopbm failed\n");
		    return -1;
		}
				
		/* The new node is left node */
		io_mnode_t *n_left = (io_mnode_t *) mdb_alloc_buffer(sizeof(io_mnode_t));
			
		if (n_left == NULL){
		    enter_kdebug("unmap_subtree(): out of memory");
		}

		n_left->set_values(n->get_lo(), lo, n->get_tid(), n->get_depth(), 0);
		n->set_lo(hi);
			
			
		/* Chain in task list */
		n_left->enq_thread_list(n);
				
		/* Are we yet in a subtree which has been splitted ? */
		if (n->get_depth() > split_depth){

		    /* Chain in left node */
		    n_left->enq_subtree(n_left_end);
					
		    /* 
		     * Parent of left node is either parent of last node or last
		     * node itself 
		     */
		    
		    n_left->parent = (n_left_end->get_depth() < n_left->get_depth()) ?\
			n_left_end : n_left->parent;
					
		    /* New end of the left part of the split subtree */
		    n_left_end = n_left;
				
		}
				
		else{ 
		    
		    /* Chain left node in subtree  before n */
		    n_left->enq_subtree(n->prev);

		    /* Parent of left node is parent of right node */
		    n_left->parent = n->parent;

		    /* 
		     * Store the beginning depth of the split and the end of the
		     * left split tree
		     */
		    
		    split_depth = n->get_depth();
		    n_left_end = n_left;
		}
	    }
		
	}
	n = n->next;	  

    } /* while (n->get_depth() > n_first->get_depth() */

    return 0;
    
}

/* 
 * int grant_subtree()
 * 
 * copies the subtree below root valid for granting into a new subtree and
 * returns new root
 */

static io_mnode_t *grant_subtree(io_mnode_t *root, io_mnode_t *dest, tcb_t *to, dword_t lo, dword_t hi){

    io_mnode_t *n, *new_root, *n_new, *n_cur;
	

    /* Allocate new root */
    new_root = n_cur = n_new =	(io_mnode_t *) mdb_alloc_buffer(sizeof(io_mnode_t));

    if (new_root == NULL){
	panic("grant_subtree(): out of memory");
    }
	
    /* Setup new root: */
    new_root->set_values(lo, hi, to->myself, root->get_depth(), 0);
    new_root->parent = root->parent;

    /* Chain in new root in thread list after dest */
    new_root->enq_thread_list(dest->next_st);
	
    /* Grant Subtree */
    n = root->next;

    while (n->get_depth() > root->get_depth()){
	if (n->get_lo() < new_root->get_hi() && n->get_hi() > new_root->get_lo()){

	    /* Allocate new children */
	    n_new = (io_mnode_t *) mdb_alloc_buffer(sizeof(io_mnode_t));
			
	    if (n_new == NULL){
		panic("grant_subtree(): out of memory");
	    }

	    /* Setup new child */
	    n_new->set_values(MAX(n->get_lo(), new_root->get_lo()),\
			      MIN(n->get_hi(), new_root->get_hi()),\
			      n->get_tid(),\
			      n->get_depth(),\
			      0);
	    n_new->parent = new_root;

			
	    /* Chain in in created subtree */
	    n_new->prev = n_cur;
	    n_cur->next = n_new;
			
	    /* 
	     * Chain in in thread list after n 
	     * At the moment, we have two mapping nodes representing the same
	     * region:
	     * the old in the subtree below root and the new one below
	     * new_root. 
	     * We unmap the old one later. But as the old subtree could be
	     * splitted, we might then have to switch the new nodes in the
	     * thread list later (*). 
	     */
	    n_new->enq_thread_list(n->next_st);
	    n_cur = n;
			
	}
	n = n->next;
	       
    }
	
	
    /* OK, now unmap the old one */
	
    /* Parent of new child will be root->prev */
    root = root->prev;
					
    /* Unmap subtree below parent */
    if (unmap_subtree(root->next, lo, hi, FLUSH_OWN_SPACE)){
	printf("grant_subtree(): unmap_subtree failed\n");
	return NULL;
    }
	
    /* Chain in created subtree in mapping tree */
    new_root->prev = root;
    n_new->next = root->next;
    root->next = new_root;
    n_new->next->prev = n_new;
	

    /* 
     * (*) Now parse the new subtree and switch nodes in thread list 
     * Actually, this is not very nice, but it works
     */
    n = new_root;

    do{
	if (n->prev_st->get_hi() > n->get_lo()){
			
	    /* Found a split subtree, therefore switch those two nodes */
	    io_mnode_t *tmp = n->prev_st;			       
			
	    tmp->prev_st->next_st = n;
	    n->next_st->prev_st = tmp;
			
	    tmp->next_st = n->next_st;
	    n->next_st = tmp;
			
	    n->prev_st = tmp->prev_st;
	    tmp->prev_st = n;
			
	}
	n = n->next;
		
		
    } while(n->get_depth() > new_root->get_depth());

	

    return new_root;
}

/*
 * void io_mdb_init()
 */

void io_mdb_init(void){
	
    // printf("io_mdb_init()");
    /* Store the virtual address of the IOPBM  */
    iopbm_virt = (dword_t) __tss.io_bitmap;
	
    /* Get physical Address of the default IOPBM */
    kspace = get_kernel_space();

    /* Get physical address of the default IOPBM */
    iopbm_phys = get_iopbm(kspace);
	
    /* This is the Pagetable containing the entry to the IOPM */
    iopbm_pgt  = kspace->pgent(pgdir_idx(iopbm_virt))->subtree(kspace,0);


#if defined(CONFIG_IA32_FEATURE_PGE)
    /* Set the pagetable entries for IOPBM to non-global if PGE enabled */
    iopbm_pgt->next(kspace, pgtab_idx(iopbm_virt))->set_flags(kspace, 0, PAGE_VALID | PAGE_WRITABLE);
    iopbm_pgt->next(kspace, pgtab_idx(iopbm_virt + 0x1000))->set_flags(kspace, 0, PAGE_VALID | PAGE_WRITABLE);

    /* Flush the corresponding TLB entries */
    flush_tlbent((ptr_t)iopbm_virt); 
    flush_tlbent((ptr_t)(iopbm_virt + 0x1000));

#endif
	
    /* Set the default IOPBM Bits to the default value */
	
    for (int i=0; i < 2048; i++)
	((ptr_t)(iopbm_virt))[i] = IOPBM_DEFAULT_VALUE;

#if defined(CONFIG_ENABLE_PVI)
    /* Enable PVI Bit */
#warning Setting PVI bit in CR4 will not work with vmware
    enable_pvi();
#endif

    /* Clear lock */
    io_mdb_lock = 0;
}

/* 
 * void io_mdb_init_sigma0();
 * 
 * implicit map of 64k IO-Space to sigma0 on its creation
 * (we can't do this during io_mdb_init as no sigma0 exists at this time)
 *
 */
void io_mdb_init_sigma0(void){
    

    /* Dummy node */
    io_mnode_t *n = (io_mnode_t *) mdb_alloc_buffer(sizeof(io_mnode_t));
	
    if (n == NULL){
	enter_kdebug("io_mdb_task_new(): out of memory");
    }

    n->set_values(0, 0, L4_SIGMA0_ID, 0, 1);

	
    /* dummy node is beginning of io_space */
    set_iospace(sigma0, n);
	
    n->parent = NULL;

    io_mnode_t *start = &sigma0_io_space;
    io_mnode_t *end   = &io_mdb_end;

    /* Setup our initial sigma0-node */
    start->set_values(0, (1<<16), L4_SIGMA0_ID, 0, 0);
    start->parent = start;
	
    /* Setup the IO-MDB Terminator */
    end->set_values(0, 0, L4_SIGMA0_ID, 0, 0);


    /* Enqueue in Subtree */
    start->next = end;
    end->prev = start;
    start->prev = end->next = NULL;

    /* Enqueue in Thread List */
    start->prev_st = start->next_st = n; 
    n->next_st = n->prev_st = start;
	
    dword_t ufl = get_user_flags(sigma0);
    /* Set IOPL to 3  */
    ufl |= (3 << 12);
    set_user_flags(sigma0, ufl);
}


/* 
 * void io_mdb_task_new();
 * 
 * dummy node creation
 * implicit map of 64k IO-Space (if configured)
 *
 */

void io_mdb_task_new(tcb_t *pager, tcb_t *task){
	
    /* Dummy node */
    io_mnode_t *n = (io_mnode_t *) mdb_alloc_buffer(sizeof(io_mnode_t));
	
    if (n == NULL){
	enter_kdebug("io_mdb_task_new(): out of memory");
    }

    n->set_values(0, 0, task->myself, 0, 1);

    /* dummy node is beginning of io_space */
    set_iospace(task, n);
	
    n->next_st = n->prev_st = n;

    n->parent = NULL;
	
#if defined(CONFIG_AUTO_IO_MAP)

    /* Create implicit mapping node n_i */
    io_mnode_t *c;
    c = get_iospace(pager);

    /* 
     * Check: 
     * if the calling task is already protected from accessing IO-Ports,
     * the new task will not get access to any ports at all.
     */
	
    if (c->get_lo() != 0 || c->get_hi() != (1 << 16))
	return;
	
    io_mnode_t *n_i = (io_mnode_t *) mdb_alloc_buffer(sizeof(io_mnode_t));
	
    if (n_i == NULL){
	enter_kdebug("io_mdb_task_new(): out of memory");
    }
	
    n_i->set_values(0, (1<<16), task->myself, c->get_depth() + 1, 0);
    n_i->parent = n;

    /* Paseeren */
    io_mdb_p();	
	
    /* Chain in in Subtree */
    n_i->enq_subtree(c);
	
    /* Verlaaten */
    io_mdb_v();	
	
    /* Chain in Task List (before n, which is after n) */
    n_i->enq_thread_list(n);
	
    dump_io_mdb();
    enter_kdebug();
#endif
}

/* 
 * void io_mdb_task_delete();
 * 
 * removing crap on task deletion
 * 
 * TODO: unmap 64K !
 */

void io_mdb_delete_iospace(tcb_t *tcb){

	
    release_iopbm(tcb);	  

    io_mnode_t *n = get_iospace(tcb);

    if (n == NULL)
	return;

    while (!n->get_dummy()){  
	n->deq_subtree();
	n = n->next_st;	  
	mdb_free_buffer((ptr_t) n->prev_st, sizeof(io_mnode_t));

    }  
    /* Free dummy node */
    mdb_free_buffer((ptr_t) n, sizeof(io_mnode_t));
	
    set_iospace(tcb, (io_mnode_t *) NULL);
}

/*
 * int map_io_fpage()
 * 
 * maps an IO-Fpage
 * 
 * returns  0 on success
 *	   -1 on error
 */

int map_io_fpage(tcb_t *from, tcb_t *to, fpage_t io_fp){

    io_mnode_t *parent, *dest, *tmp;
    dword_t hi, lo;
	
    if (IOFP_SIZE(io_fp.raw) > (1<<16)){
	printf("map_io_fpage(): wrong size\n");
	return -1;
    }

    /* Get mappers and mappees first io node and jump over dummy node */
    parent = get_iospace(from);
    dest = get_iospace(to);
	
    /* paseeren */
    io_mdb_p();


    while(!parent->get_dummy()){

	lo = IOFP_PORT(io_fp.raw);
	hi = lo + IOFP_SIZE(io_fp.raw);
	    
	/* is parent an associated mapping node to fpage (i.e. is their intersection not empty) ? */
	if (parent->get_lo() < hi){

	    if (parent->get_hi() > lo){
				
		// printf("map_io_fpage(): parent->get_lo = %x, parent->get_hi =
		// %x\n",parent->get_lo(), parent->get_hi());
		

		/* Check parents, to avoid recursive mappings */
		for (io_mnode_t *p = parent; p->get_tid().x0id.task != L4_SIGMA0_ID.x0id.task; p = p->parent){
		    if (p->get_tid().x0id.task == to->myself.x0id.task){
			printf("map_io_fpage(): no recursive mappings");
		    }
		    break;
		}
			

		/* Calculate the actual lower and higher bound to map */
		lo = MAX(IOFP_PORT(io_fp.raw), parent->get_lo());
		hi = MIN((IOFP_PORT(io_fp.raw) + IOFP_SIZE(io_fp.raw)), parent->get_hi());

		/* 
		 * Unmap region out of destination's IO-AS and search for the
		 * entry of the new mapping
		 */
		

		while(!dest->get_dummy()){
		    if (dest->get_lo() < hi){
			/* Could be that we loose the node, save next */
			tmp = dest->next_st;
			if (dest->get_hi() > lo){
			    // printf("map_io_fpage: unmap_subtree()\n");
			    if (unmap_subtree(dest, lo, hi, FLUSH_OWN_SPACE)){
				printf("map_io_fpage(): unmap_subtree failed\n");
				return -1;
			    }
			}
			dest = tmp;
		    }
		    else break;
		}
				
		/* The entry of the new mapping node */
		dest = dest->prev_st;
				
		io_mnode_t *child; 
				
		if (io_fp.fpage.grant){
		    /* grant subtree */
					
		    // printf("map_io_fpage(): grant operation\n");
		    if ((child = grant_subtree(parent, dest, to, lo, hi)) == NULL){
			printf("map_io_fpage(): grant_subtree failed\n");
			return -1;
		    }
					
		} 
				
		else{ 
		    /* Map */
					
		    /* Create the mapping */
		    child = (io_mnode_t *) mdb_alloc_buffer(sizeof(io_mnode_t));
					
		    if (child == NULL){
			panic("map_io_fpage(): out of memory");
		    }
					
		    /* Setup child */
		    child->set_values(lo, hi, to->myself, parent->get_depth() + 1, 0);
		    child->parent = parent;
		    child->enq_subtree(parent);
					
		    /* Hook child into mappees thread list after dest */
		    child->enq_thread_list(dest->next_st);
					
		}
				

		/* Zero IOPBM */
		if (zero_iopbm(to, child->get_lo(), child->get_hi() - child->get_lo())){
		    printf("map_io_fpage(): zero_iopbm failed\n");
		    return -1;
					
		}
				
				
	    } /* if (parent->get_hi() > lo) */
			
	    /* Next mapping node */
	    parent = parent->next_st;		      
			
	} /* if (parent->get_lo() < hi) */
		
	/* if parent node begins after fpage, done */
	else break;
    }		
	
    dump_io_mdb();
	
    /* Verlaaten */
    io_mdb_v();
	
    return 0;
}
/*
 * int unmap_io_fpage()
 * 
 * unmaps an IO-Fpage
 * 
 */

void unmap_io_fpage(tcb_t *from, fpage_t io_fp, dword_t mapmask){

    io_mnode_t *parent, *tmp;
    dword_t hi, lo;
	

    lo = IOFP_PORT(io_fp.raw);
    hi = lo + IOFP_SIZE(io_fp.raw);

    if (IOFP_SIZE(io_fp.raw) > (1<<16)){
	printf("unmap_io_fpage(): wrong size\n");
	return;
    }

	
    /* Get mappers first io node */
    parent = get_iospace(from);

    // printf("unmap_io_fpage: lo = %x, hi = %x\n", lo, hi);
	
    /* Paseeren */
    io_mdb_p();
	
    while(!parent->get_dummy()){
		
	/* is parent an associated mapping node ? */
	if (parent->get_lo() < hi){
			
	    /*	Could be that we loose parent */
	    tmp = parent->next_st;
	    if (parent->get_hi() > lo){
		// printf("unmap_io_fpage: parent->get_lo = %x, parent->get_hi =
		// %x\n", parent->get_lo(), parent->get_hi());
		if (unmap_subtree(parent, lo, hi, mapmask)){
		    // printf("unmap_io_fpage(): unmap_subtree failed\n");
		    return;
		}
	    }
	    parent = tmp;
	}
		
	else break;
    }
	
	
    dump_io_mdb();
    /* Verlaaten */
    io_mdb_v();
	
}
		
#endif /* (CONFIG_IO_FLEXPAGES) */
