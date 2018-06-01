#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>


#define PROC1_VADDR	0x40000000
#define PROC1_VPNO      0x40000
#define PROC2_VADDR     0x80000000
#define PROC2_VPNO      0x80000
#define TEST1_BS	1

void proc1_test1(char *msg, int lck) {
	char *addr;
	int i;

	get_bs(TEST1_BS, 100);

	if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}

	addr = (char*) PROC1_VADDR;
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'A' + i;
	}

	sleep(6);

	for (i = 0; i < 26; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	xmunmap(PROC1_VPNO);
	return;
}

void proc1_test2(char *msg, int lck) {
	int *x;

	kprintf("ready to allocate heap space\n");
	x = vgetmem(1024);
	kprintf("heap allocated at %x\n", x);
	*x = 100;
	*(x + 1) = 200;

	kprintf("heap variable: %d %d\n", *x, *(x + 1));
	vfreemem(x, 1024);
}

void proc1_test3(char *msg, int lck) {

	char *addr;
	int i;

	addr = (char*) 0x0;

	for (i = 0; i < 1024; i++) {
		*(addr + i * NBPG) = 'B';
	}

	for (i = 0; i < 1024; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	return;
}
void proc4_test1(char *msg, int lck){
	  char *x; 
          char temp; 
          get_bs(4, 100); 
          xmmap(7000, 4, 100);     
          x = 7000*4096; 
          *x = 'Y';                            
          temp = *x;      
		  kprintf("\nTemp value is:%c in Process 2",temp);
          xmunmap(7000);
	      sleep(6);
	      return;
}
void proc4_test2(char *msg, int lck){
	char *x; 
     char temp_b; 
     xmmap(6000, 4, 100); 
	sleep(2);
     x = 6000 * 4096; 
     temp_b = *x;   /* Surprise: Now, temp_b will get the value 'Y' written by the process A to this backing store '4' */
	kprintf("\nTemp value is:%c in Process 2",temp_b);
	return;
}

int main() {
	int pid1;
	int pid2;

	kprintf("\n1: shared memory\n");
	pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 0, NULL);
	resume(pid1);
	sleep(10);

	kprintf("\n2: vgetmem/vfreemem\n");
	pid1 = vcreate(proc1_test2, 2000, 100, 20, "proc1_test2", 0, NULL);
	kprintf("pid %d has private heap\n", pid1);
	resume(pid1);
	sleep(3);

/*	kprintf("\n3: Frame test\n");
	pid1 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
	resume(pid1);
	sleep(3);*/
	kprintf("\n4: Shared BS\n");
        pid1 = create(proc4_test1, 2000, 20, "proc4_test1", 0, NULL);
	pid2 = create(proc4_test2, 2000, 20, "proc4_test2", 0, NULL);
	resume(pid1);
        resume(pid2);
        sleep(3);
}
