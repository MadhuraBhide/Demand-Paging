/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

int check_processes(int store);
/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	//Initialize all backing stores
	int i;
	for(i=0;i<TOTAL_BACKING_STORES;i++){
		bsm_tab[i].bs_status = UNMAPPED;         
  		bsm_tab[i].bs_pid = -1;                           
        	bsm_tab[i].bs_vpno = -1;
    		bsm_tab[i].bs_npages = 0;                    
  		bsm_tab[i].bs_sem = -1;
  		bsm_tab[i].bs_PrivateHeap = 0;
	}
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	//Private heap Flag is passed in 'avail' pointer. 
	int i;
	//If private heap is requested return UNMAPPED backing store in avail
	if(*avail == PRIVATE_HEAP){
        	for(i=0;i<TOTAL_BACKING_STORES;i++){
			if(bsm_tab[i].bs_status == UNMAPPED){
				*avail =  i;
				return OK;
			}
		}
	}
	//Else return UNMAPPED or shared backing store
	else{
		 for(i=0;i<TOTAL_BACKING_STORES;i++){
                        if(bsm_tab[i].bs_status == UNMAPPED){
                                *avail =  i;
                                return OK;
                        }
                }
		for(i=0;i<TOTAL_BACKING_STORES;i++){
		 	if(bsm_tab[i].bs_status == MAPPED && bsm_tab[i].bs_PrivateHeap == 0){
				*avail = i;
				return OK;
		 	}
		}
	}
	*avail = -1;
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	//Free i th backing store
	if(is_Invalid_BS(i)) return SYSERR;
	bsm_tab[i].bs_status = UNMAPPED;
        bsm_tab[i].bs_pid = -1;
        bsm_tab[i].bs_vpno = -1;
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_sem = -1;
       	bsm_tab[i].bs_PrivateHeap = 0;
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{	
	//Calculate vpno from given virtual address
	int vpage_number = (vaddr/NBPG)&0x000fffff;
	int i;
	//Traverse backing stores to find vpno entry
	for(i=0;i<TOTAL_BACKING_STORES;i++){
		if(proctab[pid].BackingStores[i].bs_status == MAPPED){
		    if(proctab[pid].BackingStores[i].bs_vpno <= vpage_number && (proctab[pid].BackingStores[i].bs_vpno + proctab[pid].BackingStores[i].bs_npages) > vpage_number){
			*store = i;
			//offset in backing store
			*pageth = (vpage_number - proctab[pid].BackingStores[i].bs_vpno);
			return OK;
		    }
		}
	}
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	//check corner cases
	if(is_Invalid_BS(source) || is_Invalid_NPG(npages)){
		return SYSERR;
	}
	//if BS is UNMAPPED, set all the table variables
	if(bsm_tab[source].bs_status == UNMAPPED){
		bsm_tab[source].bs_status = MAPPED;	
		bsm_tab[source].bs_vpno = vpno;
		bsm_tab[source].bs_npages = npages;
		bsm_tab[source].bs_pid = pid;
		proctab[pid].BackingStores[source].bs_vpno = vpno;
		proctab[pid].BackingStores[source].bs_npages = npages;
	}
	//Else check if the BS contains Private Heap
	else{   
		if(bsm_tab[source].bs_PrivateHeap == 1) return SYSERR;
		//If BS is to be shared, requested npages should not be more than existing ones
		if(bsm_tab[source].bs_npages < npages) return SYSERR;
		//If BS can be shared assign vpno and previously mapped npages
		proctab[pid].BackingStores[source].bs_vpno = vpno;
		proctab[pid].BackingStores[source].bs_npages = bsm_tab[source].bs_npages;
	}
	proctab[pid].BackingStores[source].bs_PrivateHeap = 0;
        proctab[pid].BackingStores[source].bs_status = MAPPED;
	
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	//Check if vpno is valid
	if(vpno < NBPG) {
		return SYSERR;
	}
	//Lookup the vpno to find store and pageth
	int store,pageth,i;
	if(bsm_lookup(pid, vpno*NBPG , &store, &pageth)== SYSERR){
		return SYSERR;
	}
	//Clear entries from proctab
	proctab[pid].BackingStores[store].bs_PrivateHeap = 0;
        proctab[pid].BackingStores[store].bs_status = UNMAPPED;
        proctab[pid].BackingStores[store].bs_npages = 0;
        proctab[pid].BackingStores[store].bs_pid = -1;
        proctab[pid].BackingStores[store].bs_vpno = -1;
	//if BS contains private heap, free the BS
	if(flag == PRIVATE_HEAP){
		free_bsm(store);
	}
	else{	
		//check how many processes are holding the BS
		int processes =  check_processes(store);
		//If 0 processes are holding BS, free the BS
		if(processes == 0)free_bsm(store);
		else{
			//Else update pid value 
			if(bsm_tab[store].bs_pid == pid){
                             for(i=0;i<NPROC;i++){
                                 if(proctab[i].BackingStores[store].bs_status == MAPPED){
                                         bsm_tab[store].bs_pid = i;
                                 }
                             }
                        }
		}
	}
	return OK;
}
//Function to count number of processes holding a backing store
int check_processes(int store){
	int i,proc = 0;
	for(i=0;i<NPROC;i++){
		if(proctab[i].BackingStores[store].bs_status == MAPPED){
			proc++;
		}
	}
	return proc;
}
