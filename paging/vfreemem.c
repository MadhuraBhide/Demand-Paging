/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
//Reference : Referred to freemem.c in given Xinu Skeleton code
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	kprintf("");
	STATWORD ps;
        struct  mblock  *p, *q;
        unsigned top;
	
	//unsigned int *base_addr = (unsigned int *)0x00800000;
	//unsigned int *limit_addr = (unsigned int *)0x00ffffff;


        if (size==0)
                return(SYSERR);
	
        size = (unsigned)roundmb(size);
        disable(ps);
/*        for( p=proctab[currpid].vmemlist->mnext,q=&proctab[currpid].vmemlist;
             p != (struct mblock *) NULL && p < block ;
             q=p,p=p->mnext )
                ;
//        if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= &proctab[currpid].vmemlist) ||
//            (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
//                restore(ps);
//                return(SYSERR);
//        }
        if ( q!= &proctab[currpid].vmemlist && top == (unsigned)block )
                        q->mlen += size;
        else {
                block->mlen = size;
                block->mnext = p;
                q->mnext = block;
                q = block;
        }
        if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
                q->mlen += p->mlen;
                q->mnext = p->mnext;
        }
	*/
	proctab[currpid].Offset =  proctab[currpid].Offset - size;
        restore(ps);
        return(OK);
}
