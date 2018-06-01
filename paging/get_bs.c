#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
  
    STATWORD ps;
    disable(ps);
    if(is_Invalid_BS(bs_id) || is_Invalid_NPG(npages)){
	restore(ps);
	return SYSERR;	
    }
    if(bsm_tab[bs_id].bs_status == UNMAPPED){
	restore(ps);
	return npages;	
    }
    else if(bsm_tab[bs_id].bs_status == MAPPED){
	if(bsm_tab[bs_id].bs_PrivateHeap == 1){
		restore(ps);
                return SYSERR;
	}
        else{
    		restore(ps);
    		return bsm_tab[bs_id].bs_npages;
	}
    }
}
