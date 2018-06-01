/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

void clear_frame_mappings(int virtpage,int pid,int store);
/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);
  if(is_Invalid_BS(source) || is_Invalid_NPG(npages) || virtpage < 4096){
	restore(ps);
	return SYSERR;	
  }
  if (bsm_map(currpid, virtpage, source, npages) == SYSERR){
	restore(ps);
	return SYSERR;
  }
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  STATWORD ps;
  disable(ps);
  if(virtpage < 4096){
        restore(ps);
        return SYSERR;
  }
  int test = virtpage*NBPG;
  pd_t *pd = proctab[currpid].pdbr + 4*(test>>22);
  int store,pageth,flag;
  if(bsm_lookup(currpid,virtpage*NBPG,&store,&pageth) == SYSERR){
	 return SYSERR;
  }
  flag = proctab[currpid].BackingStores[store].bs_PrivateHeap;
  if(flag == PRIVATE_HEAP){
	 proctab[currpid].vmemlist = NULL;
	 release_bs(store);	
  }
  else clear_frame_mappings(virtpage,currpid,store);
  if (bsm_unmap(currpid, virtpage, proctab[currpid].BackingStores[store].bs_PrivateHeap) == SYSERR) {
	 restore(ps);
	return SYSERR;
   }
   write_cr3(proctab[currpid].pdbr);
   restore(ps);
   return OK;
}

void clear_frame_mappings(int virtpage,int pid,int store){
	int i,vaddr;
	while(virtpage < (proctab[pid].BackingStores[store].bs_vpno+proctab[pid].BackingStores[store].bs_npages)){
        	for(i=0;i<NFRAMES;i++){
                	if(frm_tab[i].fr_status == MAPPED && frm_tab[i].fr_type == FR_PAGE && frm_tab[i].fr_pid == pid){
                              if(frm_tab[i].fr_vpno == virtpage){
					free_frm(i);
				}
                        }
                }
		virtpage++;
	}
}
