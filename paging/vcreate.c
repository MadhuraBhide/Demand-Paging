/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

LOCAL	newpid();

//Reference : Referred create.c in given Xinu code skeleton

SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);
	int backing_store,pid;
	if(is_Invalid_NPG(hsize)) {
		restore(ps);
		return SYSERR;	
	}
	int avail = PRIVATE_HEAP;
	if(get_bsm(&avail) == -1){
                restore(ps);
		return SYSERR;
	}
	backing_store = avail;
	//Create a process using normal create API
	pid =  create(procaddr,ssize,priority,name,nargs,args);
	//Map the pid to available BS
	if (bsm_map(pid, 4096, backing_store, hsize) == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	bsm_tab[backing_store].bs_PrivateHeap = 1;
	proctab[pid].BackingStores[backing_store].bs_PrivateHeap = 1;
	proctab[pid].store =  backing_store;
	proctab[pid].vhpnpages = hsize;
	proctab[pid].vhpno = 4096; 
	proctab[pid].Offset = 0;
	proctab[pid].vmemlist->mnext = (struct mblock *)(BACKING_STORE_BASE + (backing_store<<19)); 
	proctab[pid].vmemlist->mnext->mlen  = hsize * NBPG; 

	restore(ps);
	return(pid);
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
