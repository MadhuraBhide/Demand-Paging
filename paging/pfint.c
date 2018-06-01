/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

void update_PageReplacement_DS(int frameID);
/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  STATWORD ps;
  disable(ps);
  int store,pageth;
  unsigned long fault_addr;
 // Get the faulted address a
  fault_addr = read_cr2();
  //Check that a is a legal address (i.e. that it has been mapped to a backing store). If it is not, print an error message and kill the process
  if(bsm_lookup(currpid, fault_addr, &store, &pageth) == SYSERR){
	kprintf("\nThis is not a legal address: %u",fault_addr);
	kill(currpid);
	restore(ps);
	return(SYSERR);
  }
  //Let pt point to the p-th page table. If the p-th page table does not exist, obtain a frame for it and initialize it.
  unsigned long pg_tb = fault_addr >> 22;
  pd_t *pd = proctab[currpid].pdbr + 4*pg_tb;
  unsigned long pg_no = (fault_addr >> 12) & 0x000003ff;
  //Check for a faulted address, page table has been created
  if(pd->pd_pres != 1){
	//If not create new page table
  	int pt_frame = new_Page_Table(currpid);                     //Write this function
	if(pt_frame == SYSERR) return SYSERR;
	pd->pd_pres = 1;
	pd->pd_write = 1;
	pd->pd_base = FRAME0 + pt_frame;
  }
  //In the inverted page table, increase the reference count of the frame that holds pt. This indicates that one more of pt's entries is marked as "present."
  pt_t *pt =  pd->pd_base*NBPG + 4*pg_no ;
  frm_tab[(unsigned int)pt/NBPG - FRAME0].fr_refcnt++;
  int avail;
  //Obtain an empty frame
  get_frm(&avail);                        
  int new_frame = avail;
  if(new_frame == SYSERR){
  	kill(currpid);
	restore(ps);
	return(SYSERR);
  }
  //Update page table entries for new frame
  //Update pt to mark the appropriate entry as present and set any other fields. Also set the address portion within the entry to point to frame f.
  pt->pt_pres = 1;
  pt->pt_write =1;
  pt->pt_base = FRAME0 + new_frame;
  
  //Update inverted page table entries
  frm_tab[new_frame].fr_status = MAPPED;
  frm_tab[new_frame].fr_pid = currpid;
  frm_tab[new_frame].fr_vpno = (fault_addr/NBPG) & 0x000fffff;
  frm_tab[new_frame].fr_type = FR_PAGE;
  frm_tab[new_frame].fr_dirty = 0;
  //Copy data from BS in the new frame for the process
  read_bs((new_frame+FRAME0)*NBPG,store,pageth);
  update_PageReplacement_DS(new_frame);                               //Write this function
  //Refresh TLB
  write_cr3(proctab[currpid].pdbr);  
  restore(ps);
  return OK;
}
void update_PageReplacement_DS(int frameID){
  //update circular queue for SC
  if(page_replace_policy == SC){
        SCQueue *newNode = (struct SCQueue*)getmem(sizeof(SCQueue));
        newNode->frameID = frameID;
	//Insert data frames in circular queue
        if(SChead == NULL) {
              SChead = newNode;
              clk_ptr = SChead;
        }
        if(SCtail != NULL) SCtail->nextFrame = newNode;
        SCtail = newNode;
        SCtail->nextFrame = SChead;
   }
   else if(page_replace_policy == LFU){
	//Update refcnt variable for LFU
  	int i;
        for(i=0;i<NFRAMES;i++){
                if(frm_tab[i].fr_status == MAPPED && frm_tab[i].fr_type == FR_PAGE){
                                pd_t *pd = proctab[frm_tab[i].fr_pid].pdbr + 4*(frm_tab[i].fr_vpno >> 10);
                                pt_t *pt = pd->pd_base*NBPG + 4*(frm_tab[i].fr_vpno & 0x000003ff) ;
                                if(pt->pt_acc == 1) frm_tab[i].fr_refcnt++;
				pt->pt_acc = 0;
                        }
                }
        }		
}        
