#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <3ds/svc.h>
#include <kernel/kproc.h>
#include <dma.h>


u32 current_kpAddr,
    kpAddrSelf,
	dummy,
	key;

u32 *kProcessPtr = NULL;
Result ret;

u8 dummyData[0x5000] = {0x00, 0xFF, 0xBE, 0xBA};

bool choice = true;

scenic_kproc *kpHome;

scenic_process *pHome,
               *pSelf;

//***********************************************//

u32 wait_key(void) {
	hidScanInput();
	u32 old_kDown, kDown = 0;
	old_kDown = hidKeysDown();

	while (aptMainLoop()) {
		gspWaitForVBlank();

		hidScanInput();
		kDown = hidKeysDown();
		if (kDown != old_kDown)
			break;

		gfxFlushBuffers();
		gfxSwapBuffers();
	}
	return kDown;
}

//***********************************************//

s32 dump_k(void) {
    __asm__ volatile ("cpsid aif");
	current_kpAddr = *(u32 *)0xffff9004;
	return 0;
}

void dump(void) {
    svcBackdoor(dump_k);
}

//***********************************************//

s32 write_k(void) {
    __asm__ volatile ("cpsid aif");
	*(u32 *)0xffff9004 = (u32)kProcessPtr;
	return 0;
}


void write(void) {
    svcBackdoor(write_k);
}

//***********************************************//

s32 restore_k(void) {
    __asm__ volatile ("cpsid aif");
	*(u32 *)0xffff9004 = kpAddrSelf;
	return 0;
}


void restore(void) {
    svcBackdoor(restore_k);
}

//***********************************************//

s32 patchErrorCode_k(void) {
    __asm__ volatile ("cpsid aif");
	*(u32 *)0xdff88468 = (u32)0;
	*(u32 *)0xdff8846c = (u32)0;
	return 0;
}


void patchErrorCode(void) {
    svcBackdoor(patchErrorCode_k);
}

//***********************************************//

void readMemory(void) {
    dummy = 0;
	ret = dma_copy(pSelf, (void *)&dummy, pHome, (void*)0x6000000, (u32)0x4);
	printf("dma_copy[2] returned: %08x\n", ret);
	printf("remoteMemory value: %08x\n", dummy);
	printf("Press X to read memory again\n");
}

//***********************************************//

int main()
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	printf("-Press A to Inject data into Menu Home\n-Press B to read already injected data into Menu\n Home\n");
    
	key = wait_key();
	if(key == KEY_A) {
	    dump();
        printf("Done, current kProcess: %08x\n", current_kpAddr);
	    kpHome = kproc_find_by_id((u32)0xf);
	    printf("Done, home kProcess: %08x\n", (u32)kpHome->ptr);
	
	    kpAddrSelf = current_kpAddr;
	    kProcessPtr = kpHome->ptr;
	
	    patchErrorCode();
	    write();
	    ret = svcControlMemory((u32 *)&dummy, (u32)0x6000000, NULL, (u32)0x5000, MEMOP_ALLOC | MEMOP_REGION_SYSTEM, MEMPERM_READ | MEMPERM_WRITE);
        restore();
	    printf("svcControlMemory returned: %08x\n", ret);
	
	    pHome = proc_open((u32)0xf, FLAG_NONE);
	    pSelf = proc_open((u32)-1, FLAG_NONE);
	    ret = dma_protect(pHome, (void *)0x6000000, 0x5000);
	    printf("dma_protect returned: %08x\n", ret);
	    ret = dma_copy(pHome, (void*)0x6000000, pSelf, (void *)&dummyData, (u32)0x5000);
	    printf("dma_copy[1] returned: %08x\n", ret);
	    svcSleepThread(100*1000*1000);
	    readMemory();
	}
    else if(key == KEY_B) {
	    pHome = proc_open((u32)0xf, FLAG_NONE);
	    pSelf = proc_open((u32)-1, FLAG_NONE);
	    readMemory();
	}



	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		//Your code goes here

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; //Break in order to return to hbmenu

		if (kDown & KEY_X) readMemory();
		
		// Flush and swap frame-buffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	gfxExit();
	return 0;
}