/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int new_Page_Table(int pid);
int Page_Replacement_SC();
int Page_Replacement_Aging();
int create_Page_Directory(int pid);
int Initialize_global_PageTables();
int global_Pages[4];
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  //Initialize frm_tab
  int i;
  for(i=0;i<NFRAMES;i++){
  	frm_tab[i].fr_status = UNMAPPED;
	frm_tab[i].fr_pid = -1;
	frm_tab[i].fr_vpno = -1;
	frm_tab[i].fr_refcnt = 0;
	frm_tab[i].fr_type = -1;
	frm_tab[i].fr_dirty = 0;	
  }
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  //Traverse inverted page table to selected UNMAPPED frame and return its index to a prcess
  int i;
  for(i=0;i<NFRAMES;i++){
	if(frm_tab[i].fr_status	== UNMAPPED){
		*avail = i;
		return OK;
	}
  }
  //if no such frame exists, obtain frame ID by applying frame replacement logic
  int frame_number;
  if( grpolicy() == SC) frame_number =  Page_Replacement_SC();				//Write these functions
  else frame_number =  Page_Replacement_Aging();
  *avail = frame_number;
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
//Free a frame
  if(is_Invalid_NPG(i)) return SYSERR;
  /*if frame ID returned is a page directory, free all the page tables which are present
  (excpet for the global ones, which reside in first 4 entries of page directory)
  */
  if(frm_tab[i].fr_type == FR_DIR){
   	int k;
	//Scan all 1024 page table entries and free all the page tables which are present
	for(k=4;k<1024;k++){
		pd_t *pd = proctab[frm_tab[i].fr_pid].pdbr + 4*k;
		if(pd->pd_pres == 1){
			//recursive call fro free frame
			free_frm(pd->pd_base - FRAME0);
		}
	}
	//Free the page directory frame
	free_PageFrame(i);
  }
  //if frame ID returned is a page tables, free all the pages which are present
  else if(frm_tab[i].fr_type == FR_TBL) {
	int k;
	 //Scan all 1024 page entries and free all the pages which are present
        for(k=0;k<1024;k++){
                pt_t *pt = (i+FRAME0)*NBPG + 4*k;
                if(pt->pt_pres == 1){
                        free_frm(pt->pt_base - FRAME0);
                }
        }
	//Remove respective entry from page directory
	for(k=0;k<1024;k++){
		pd_t *pd = proctab[frm_tab[i].fr_pid].pdbr + 4*k;
		if(pd->pd_base - FRAME0 == i){
			free_PageDir(pd);
		}
	}
	//Free the page table frame
        free_PageFrame(i);

  }
  //If a frame contains process data, lookup respective Virtual page entry in BS
  else if(frm_tab[i].fr_type == FR_PAGE){
	char *src = (FRAME0+i)*NBPG;
	int store,pageth;
	//If the dirty bit for page vp was set in its page table you must do the following:
	//Use the backing store map to find the store and page offset within store given pid and a. If the lookup fails, something is wrong. Print an error message and kill the process pid.
	if(bsm_lookup(frm_tab[i].fr_pid, frm_tab[i].fr_vpno*NBPG, &store, &pageth) == SYSERR){
		 return SYSERR;
	}
	//Write the page back to the backing store.
	pd_t *pd = proctab[frm_tab[i].fr_pid].pdbr + 4*(frm_tab[i].fr_vpno>>10);
	pt_t *pt = pd->pd_base*NBPG + 4*(frm_tab[i].fr_vpno & 0x000003ff);
	if(pt->pt_dirty == 1) write_bs(src, store, pageth);
	if(pd->pd_pres == 0 || pt->pt_pres == 0) return SYSERR;
	//Remove respective page table entry
	free_PageTableEntry(pt);
	//Free the data frame
	free_PageFrame(i);
  }
  return OK;
}
void free_PageFrame(int i){
	pd_t *pd = proctab[frm_tab[i].fr_pid].pdbr +  4*(frm_tab[i].fr_vpno>>10);
	if(frm_tab[i].fr_type == FR_PAGE){
		//In the inverted page table decrement the reference count of the frame occupied by pt
		frm_tab[pd->pd_base - FRAME0].fr_refcnt--;
		//If the reference count has reached zero, you should mark the appropriate entry in pd as being not present. This conserves frames by keeping only page tables which are necessary.
		if(frm_tab[pd->pd_base - FRAME0].fr_refcnt == 0){
			free_frm(pd->pd_base - FRAME0);
		}
	}
	//Clear frame tab entries
	frm_tab[i].fr_status = UNMAPPED;
        frm_tab[i].fr_pid = -1;
        frm_tab[i].fr_vpno = -1;
        frm_tab[i].fr_refcnt = 0;
        frm_tab[i].fr_type = -1;
        frm_tab[i].fr_dirty = 0;
}
void free_PageTableEntry(pt_t *temp){
//	kprintf("\nFree page Table entry");
	temp->pt_pres = 0;
        temp->pt_write = 1;
        temp->pt_user = 0;
        temp->pt_pwt = 0;
        temp->pt_pcd = 0;
        temp->pt_acc = 0;
        temp->pt_dirty = 0;
        temp->pt_mbz = 0;
        temp->pt_global = 0;
        temp->pt_avail = 0;
        temp->pt_base = 0;
}
void free_PageDir(pd_t *temp){
	//kprintf("\nFree page dir");
	temp->pd_pres = 0;
        temp->pd_write = 1;
        temp->pd_user = 0;
        temp->pd_pwt = 0;
        temp->pd_pcd = 0;
        temp->pd_acc = 0;
        temp->pd_fmb = 0;
        temp->pd_mbz = 0;
        temp->pd_global = 0;
        temp->pd_avail = 0;
        temp->pd_base = 0;
}

int new_Page_Table(int pid){
   //Create new Page Table
   int avail;
   get_frm(&avail);
   int new_frame = avail;
   if(new_frame == SYSERR) return SYSERR;
   //Mark frame as MAPPED in frm tab
   frm_tab[new_frame].fr_status = MAPPED;
   frm_tab[new_frame].fr_pid = pid;
   frm_tab[new_frame].fr_vpno = -1;
   frm_tab[new_frame].fr_refcnt = 0;
   frm_tab[new_frame].fr_type = FR_TBL;
   frm_tab[new_frame].fr_dirty = 0;
   //Initialize all frames in page table
   int i;
   for(i=0;i<1024;i++){
	pt_t *temp = (FRAME0+new_frame)*NBPG + 4*i;
	free_PageTableEntry(temp);
   }
  return new_frame;
}

int create_Page_Directory(int pid){
   //create new page directory
   int avail;
   get_frm(&avail);
   int new_frame = avail;
   if(new_frame == SYSERR) return SYSERR;
   //mark frame as MAPPED in frm tab
   frm_tab[new_frame].fr_status = MAPPED;
   frm_tab[new_frame].fr_pid = pid;
   frm_tab[new_frame].fr_vpno = -1;
   frm_tab[new_frame].fr_refcnt = 4;
   frm_tab[new_frame].fr_type = FR_DIR;
   frm_tab[new_frame].fr_dirty = 0;
   proctab[pid].pdbr = (FRAME0+new_frame)*NBPG;
   //Initialize all page tables, first 4 page tables being global
   int i;
   for(i=0;i<1024;i++){
        pd_t *temp = (FRAME0+new_frame)*NBPG + 4*i;
        free_PageDir(temp);
	if(i<4){
		temp->pd_pres = 1;
		temp->pd_base = global_Pages[i];
	}
   }
  return OK;
}
//initialize.c calls this function
int Initialize_global_PageTables(){
	int i,frame_ID,j;
	//create 4 page tables
	for(i=0;i<4;i++){
		frame_ID = new_Page_Table(NULLPROC);
		//save frame number of global page table in a global array(accessed by all processes)
		global_Pages[i] = frame_ID + FRAME0;
		//all frames in these page tables are available (physical memory till 16 MB = 4 page tables)
		frm_tab[frame_ID].fr_refcnt = 1024;
		for(j=0;j<1024;j++){
			//Initialize variables for each page table
			pt_t *pt = global_Pages[i]*NBPG + j*4;
			pt->pt_write = 1;
			pt->pt_pres = 1;
			pt->pt_base = j + i*1024;
		}	
	}
	return OK;	
}
//Get frame calls this function of policy is SC
int Page_Replacement_SC(){
  if(clk_ptr == NULL){
  	return SYSERR;
  }
  unsigned long temp_vpno,temp_pdbr;
  pd_t *temp_pd;
  pt_t *temp_pt;
  //Traverse circular queue till you reach a frame for which pt->acc = 0
  while(1){
	//page should be PAGE frame only
  	if(frm_tab[clk_ptr->frameID].fr_type==FR_PAGE){
	   temp_vpno = frm_tab[clk_ptr->frameID].fr_vpno;
           temp_pdbr = proctab[frm_tab[clk_ptr->frameID].fr_pid].pdbr;
           temp_pd = temp_pdbr + 4*(temp_vpno >> 10);
           temp_pt = (temp_pd->pd_base)*NBPG + 4*(temp_vpno & 0x000003ff);
	   //if pt acc is 1, clear the bit
	   if(temp_pt->pt_acc == 1){
	   	temp_pt->pt_acc = 0;
	   }
	   //Else come out of loop
	   else{
		break;	
	   }
	}
     //keep moving clock pointer
     clk_ptr = clk_ptr->nextFrame;
  }
  SCQueue *temp = clk_ptr;
  free_frm(clk_ptr->frameID);
  clk_ptr = clk_ptr->nextFrame;
  kprintf("\nPage replacement SC returns frame : %d\n",( temp->frameID + FRAME0));
  return temp->frameID;
}
int Page_Replacement_Aging(){
	int i,frame_ID=-1,least = 65535;
	for(i=4;i<NFRAMES;i++){
		//Only look for data frames
		if(frm_tab[i].fr_type == FR_PAGE){
			//obtain a page with lowest refcnt
			if(frm_tab[i].fr_refcnt < least){
				least = frm_tab[i].fr_refcnt;
				frame_ID = i;
			}
			//in case of a tie, largest VPNO
			else if(frm_tab[i].fr_refcnt == least){
				if(frm_tab[i].fr_vpno > frm_tab[frame_ID].fr_vpno){
					frame_ID = i;
				}
			}
		}
	}
	free_frm(frame_ID);
	kprintf("\nPage replacement LFU returns frame : %d\n",(frame_ID + FRAME0));
	return frame_ID;
}
