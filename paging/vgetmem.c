/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
//Reference : Referred to getmem.c in given Xinu Skeleton code
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	STATWORD ps;
        struct  mblock  *p, *q, *leftover;
	unsigned int ret;
        disable(ps);
       // if (nbytes==0 ||  proctab[currpid].vmemlist->mnext== (struct mblock *) NULL) {
       if (nbytes==0 || proctab[currpid].store == -1 || nbytes > ((proctab[currpid].vhpnpages*NBPG) - proctab[currpid].Offset)){
                restore(ps);
                return( (WORD *)SYSERR);
        }
        nbytes = (unsigned int) roundmb(nbytes);
	ret = (proctab[currpid].vhpno*NBPG) + proctab[currpid].Offset;
        proctab[currpid].Offset =  proctab[currpid].Offset + nbytes;
       
        for (q= proctab[currpid].vmemlist,p=proctab[currpid].vmemlist->mnext ;
             p != (struct mblock *) NULL ;
             q=p,p=p->mnext)
                if ( p->mlen == nbytes) {
                        q->mnext = p->mnext;
                } else if ( p->mlen > nbytes ) {
                        leftover = (struct mblock *)( (unsigned)p + nbytes );
                        q->mnext = leftover;
                        leftover->mnext = p->mnext;
                        leftover->mlen = p->mlen - nbytes;
                }
        restore(ps);
	return( (WORD *)ret);
/*        restore(ps);
	kprintf("\nSYSERR2 in vgetmem");
        return( (WORD *)SYSERR );*/
}
