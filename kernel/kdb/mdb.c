/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     mdb.c
 * Description:   Mapping database sumping and statistics functions.
 *                
 * @LICENSE@
 *                
 * $Id: mdb.c,v 1.15 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mapping.h>
#include <kdebug.h>

static void dump_mdbmaps(mnode_t *, dword_t, dword_t,
			 rnode_t *, char *) L4_SECT_KDEBUG;
static void dump_mdbroot(rnode_t *, dword_t, dword_t, char *) L4_SECT_KDEBUG;


static inline dword_t mdb_arraysize(dword_t pgsize)
{
    return 1 << (mdb_pgshifts[pgsize+1] - mdb_pgshifts[pgsize]);
}

static inline dword_t mdb_pgsize(dword_t pgsize)
{
    return 1 << mdb_pgshifts[pgsize];
}

static inline rnode_t * index_root(rnode_t *root, dword_t pgsz, dword_t paddr)
{
    return root + ((paddr >> mdb_pgshifts[pgsz]) & (mdb_arraysize(pgsz) - 1));
}

static inline int hw_pgsize (int mdb_pgsize)
{
    int s = 0;
    while (hw_pgshifts[s] < mdb_pgshifts[mdb_pgsize])
	s++;
    return s;
}

static void dump_mdbmaps(mnode_t *map, dword_t paddr, dword_t size,
			 rnode_t *proot, char *spc)
{
    mnode_t *pmap = NULL;

    while ( map ) {
	if ( map == sigma0_mapnode )
	    printf("sigma0_mapnode (%p)\n", sigma0_mapnode);
	else
	{
	    space_t * space = map->get_space();
	    int hwsize = hw_pgsize (size);
	    printf("%s[%d] space=%p  vaddr=%p  pgent=%p  (%p)\n",
		   spc - map->get_depth()*2,
		   map->get_depth(), space,
		   pmap ? map->get_pgent(pmap)->vaddr(space, hwsize, map) :
		   map->get_pgent(proot)->vaddr(space, hwsize, map),
		   pmap ? map->get_pgent(pmap) : map->get_pgent(proot),
		   map);
	}
	
	pmap = map;
	if ( map->is_next_root() )
	{
	    dualnode_t *dual = (dualnode_t *) map->get_nextr();

	    if ( dual->root == NULL )
		printf("============> [EMPTY ROOT]\n");
	    else
		dump_mdbroot(index_root(dual->root, size-1, paddr),
			     paddr, size-1, spc - 4 - map->get_depth()*2);
	    map = dual->map;
	}
	else
	    map = map->get_nextm();
    }
}

static void dump_mdbroot(rnode_t *root, dword_t paddr,
			 dword_t size, char *spc)
{
    printf("%s%x: %d%s %s (%p)\n",
	   spc, paddr & ~((1 << mdb_pgshifts[size]) - 1),
	   (mdb_pgshifts[size] >= 20) ? 1 << (mdb_pgshifts[size] - 20) :
	   1 << (mdb_pgshifts[size] - 10),
	   (mdb_pgshifts[size] >= 20) ? "MB" : "KB",
	   root->is_next_root() ? "[ROOT]" : "[MAP] ", root);
    
    if ( root->is_next_root() )
    {
	dualnode_t *dual = (dualnode_t *) root->get_map();
	dump_mdbmaps(dual->map, paddr, size, root, spc - 4);
	dump_mdbroot(index_root(dual->root, size-1, paddr), paddr,
		     size-1,  spc - 4);
    }
    else
    {
	if ( root->get_map() != NULL )
	    dump_mdbmaps(root->get_map(), paddr, size,
			 root, spc - 4);
    }
}


void kdebug_dump_map(dword_t paddr)
{
    static char spaces[] = "                                                ";
    dualnode_t *dual = (dualnode_t *) sigma0_mapnode->get_nextr();
    putc('\n');
    dump_mdbroot(index_root(dual->root, NUM_PAGESIZES-1, paddr), paddr,
		 NUM_PAGESIZES-1, spaces + sizeof(spaces)-1);
}

#define MAX_MDB_DEPTH	5
#define MAX_MDB_MAPS	10

static dword_t num_depths[MAX_MDB_DEPTH+1] L4_SECT_KDEBUG;
static dword_t num_maps[MAX_MDB_MAPS+1] L4_SECT_KDEBUG;
static dword_t max_depth L4_SECT_KDEBUG;
static dword_t max_maps L4_SECT_KDEBUG;

static void stat_mdbroot(rnode_t *root, int size, int num)
{
    dualnode_t *dual;
    mnode_t *map;
    dword_t cnt;

    for ( ; num--; root++ )
    {
	if ( root->is_next_root() )
	{
	    dual = (dualnode_t *) root->get_map();
	    stat_mdbroot(dual->root, size-1, mdb_arraysize(size-1));
	    map = dual->map;
	}
	else
	    map = root->get_map();

	cnt = 0;
	while ( map )
	{
	    if ( map->get_depth() > max_depth )
		max_depth = map->get_depth();
	    if ( map->get_depth() > MAX_MDB_DEPTH )
		num_depths[MAX_MDB_DEPTH] += 1;
	    else
		num_depths[map->get_depth()] += 1;

	    cnt++;
	    if ( map->is_next_root() )
	    {
		dual = (dualnode_t *) map->get_nextr();
		stat_mdbroot(dual->root, size-1, mdb_arraysize(size-1));
		map = dual->map;
	    }
	    else
		map = map->get_nextm();
	}

	if ( cnt > max_maps )
	    max_maps = cnt;
	if ( cnt > MAX_MDB_MAPS )
	    num_maps[MAX_MDB_MAPS] += 1;
	else
	    num_maps[cnt] += 1;
    }
}

void kdebug_mdb_stats(dword_t start, dword_t end)
{
    dualnode_t *dual = (dualnode_t *) sigma0_mapnode->get_nextr();
    dword_t addr;
    int i, j;

    for ( i = 0; i < MAX_MDB_DEPTH; i++ )
	num_depths[i] = 0;
    for ( i = 0; i < MAX_MDB_MAPS; i++ )
	num_maps[i] = 0;
    max_depth = max_maps = 0;

    for ( i = 1, addr = start & ~(mdb_pgsize(NUM_PAGESIZES-1)-1);
	  addr < (end & ~(mdb_pgsize(NUM_PAGESIZES-1)-1));
	  addr += mdb_pgsize(NUM_PAGESIZES-1), i++ ) {}
    stat_mdbroot(index_root(dual->root, NUM_PAGESIZES-1, start),
		 NUM_PAGESIZES-1, i);

    printf("\nMapping database statistics (%p -> %p)\n", start, end);
    printf("Depth\tNum.\t\tMaps\tNum.\n");
    for ( i = j = 0; i <= MAX_MDB_DEPTH || j <= MAX_MDB_MAPS; i++, j++ )
    {
	if ( i <= MAX_MDB_DEPTH )
	    printf(" %d%c\t %d", i, i == MAX_MDB_DEPTH ? '+' : ' ',
		   num_depths[i]);
	else
	    printf("   \t");

	if ( j <= MAX_MDB_MAPS )
	    printf("\t\t %d%c\t %d\n", j, j == MAX_MDB_MAPS ? '+' : ' ',
		   num_maps[j]);
	else
	    printf("\n");
    }

    printf("\n Max:\t %d\t\t Max:\t %d\n", max_depth, max_maps);
}
