#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
extern int check_processes(int bs_id);
SYSCALL release_bs(bsd_t bs_id) {
   STATWORD ps;
   disable(ps);
  /* release the backing store with ID bs_id */
    
   if(is_Invalid_BS(bs_id)){
	return SYSERR;			   
   }
//Check how many processes are holding the BS
   int procs = check_processes(bs_id);
//If more than one processes are holding the BS, return system error
   if(procs > 1) return SYSERR;
   if(procs == 1 && bsm_tab[bs_id].bs_pid == currpid) {
//	kprintf("\nFree bsm inside release BS");
	free_bsm(bs_id);}
   proctab[currpid].BackingStores[bs_id].bs_PrivateHeap = 0;
   proctab[currpid].BackingStores[bs_id].bs_status = UNMAPPED;
   proctab[currpid].BackingStores[bs_id].bs_npages = 0;
   proctab[currpid].BackingStores[bs_id].bs_pid = -1;
   proctab[currpid].BackingStores[bs_id].bs_vpno = -1;
   write_cr3(proctab[currpid].pdbr);
   restore(ps);
   return OK;
}
