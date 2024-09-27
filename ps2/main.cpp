#include "types.h"
#include "dc/mem/_vmem.h"
#include "dc/sh4/sh4_registers.h"
#include "dc/sh4/sh4_opcode_list.h"
#include "stdclass.h"
#include "dc/dc.h"
#include "config/config.h"
#include "plugins/plugin_manager.h"
#include "cl/cl.h"
#include <chrono>

#include <sifrpc.h>
#include <loadfile.h>
#include "libpad.h"


#undef r
#undef fr

void __debugbreak() { fflush(stdout); *(int*)0=1;}

void SetApplicationPath(wchar* path);

static char padBuf[256] __attribute__((aligned(64)));
static int actuators;

/*
 * loadModules()
 */
static void
loadModules(void)
{
    int ret;


    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0) {
        printf("sifLoadModule sio failed: %d\n", ret);
    }

    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0) {
        printf("sifLoadModule pad failed: %d\n", ret);
    }
}

/*
 * waitPadReady()
 */
static int waitPadReady(int port, int slot)
{
    int state;
    int lastState;
    char stateString[16];

    state = padGetState(port, slot);
    lastState = -1;
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
        if (state != lastState) {
            padStateInt2String(state, stateString);
            printf("Please wait, pad(%d,%d) is in state %s\n",
                       port, slot, stateString);
        }
        lastState = state;
        state=padGetState(port, slot);
    }
    // Were the pad ever 'out of sync'?
    if (lastState != -1) {
        printf("Pad OK!\n");
    }
    return 0;
}


/*
 * initializePad()
 */
static int
initializePad(int port, int slot)
{

    int ret;
    int modes;
    int i;

    waitPadReady(port, slot);

    // How many different modes can this device operate in?
    // i.e. get # entrys in the modetable
    modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
    printf("The device has %d modes\n", modes);

    if (modes > 0) {
        printf("( ");
        for (i = 0; i < modes; i++) {
            printf("%d ", padInfoMode(port, slot, PAD_MODETABLE, i));
        }
        printf(")");
    }

    printf("It is currently using mode %d\n",
               padInfoMode(port, slot, PAD_MODECURID, 0));

    // If modes == 0, this is not a Dual shock controller
    // (it has no actuator engines)
    if (modes == 0) {
        printf("This is a digital controller?\n");
        return 1;
    }

    // Verify that the controller has a DUAL SHOCK mode
    i = 0;
    do {
        if (padInfoMode(port, slot, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
            break;
        i++;
    } while (i < modes);
    if (i >= modes) {
        printf("This is no Dual Shock controller\n");
        return 1;
    }

    // If ExId != 0x0 => This controller has actuator engines
    // This check should always pass if the Dual Shock test above passed
    ret = padInfoMode(port, slot, PAD_MODECUREXID, 0);
    if (ret == 0) {
        printf("This is no Dual Shock controller??\n");
        return 1;
    }

    printf("Enabling dual shock functions\n");

    // When using MMODE_LOCK, user cant change mode with Select button
    padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

    waitPadReady(port, slot);
    printf("infoPressMode: %d\n", padInfoPressMode(port, slot));

    waitPadReady(port, slot);
    printf("enterPressMode: %d\n", padEnterPressMode(port, slot));

    waitPadReady(port, slot);
    actuators = padInfoAct(port, slot, -1, 0);
    printf("# of actuators: %d\n",actuators);

    // if (actuators != 0) {
    //     actAlign[0] = 0;   // Enable small engine
    //     actAlign[1] = 1;   // Enable big engine
    //     actAlign[2] = 0xff;
    //     actAlign[3] = 0xff;
    //     actAlign[4] = 0xff;
    //     actAlign[5] = 0xff;

    //     waitPadReady(port, slot);
    //     printf("padSetActAlign: %d\n",
    //                padSetActAlign(port, slot, actAlign));
    // }
    // else {
    //     printf("Did not find any actuators.\n");
    // }

    waitPadReady(port, slot);

    return 1;
}


int main(int argc, wchar* argv[])
{
	printf("Greetings to pukko and Gustavo for the pad code\n");

	SifInitRpc(0);

	loadModules();

    padInit(0);

	int ret;
	if((ret = padPortOpen(0, 0, padBuf)) == 0) {
        printf("padOpenPort failed: %d\n", ret);
    }

    if(!initializePad(0, 0)) {
        printf("pad initalization failed!\n");
    }

	__asm__ __volatile__ ("ctc1 $0, $31");

	int rv=EmuMain(argc,argv);

	return rv;
}


int os_GetFile(char *szFileName, char *szParse,u32 flags)
{
	strcpy(szFileName, "boot.cdi");
	return 1;
}

int os_msgbox(const wchar* text,unsigned int type)
{
	printf("OS_MSGBOX: %s\n",text);
	return 0;
}

double os_GetSeconds()
{
	auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> duration = now.time_since_epoch();

	return duration.count();
}