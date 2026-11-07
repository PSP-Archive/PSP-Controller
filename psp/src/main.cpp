//
// PSP Controller
//		by Mikko Mononen 2005 <memon@inside.org>
//
// Description:
//		This program sends the status of the PSP pad at each screen update (60Hz).
//		The data is send as 16 byte packed via UDP. This program was designed to
//		let the PSP act as controller to create quick game prototypes on PC.
// Note:
//		The program is set to terminate when you press Start!
//
// Based on:
//		WiFi Simple test app .03 by psppet
//

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern "C"
{
	#include "nlh.h" // net lib helper
}

#define printf pspDebugScreenPrintf

// The settings are stored in a text file stored in the same folder as the executable.
char	g_appPath[512] = "";
u8		g_serverAddr[4] = {127, 0, 0, 1};
int		g_serverPort = 21;

bool	LoadSettings()
{
	char path[512];
	strcpy( path, g_appPath );
	strcat( path, "settings.txt" );

	FILE*	pStream = fopen( path, "r" );
	if( !pStream )
	{
		printf("Could not open settings file '%s'.\n", path );
		return false;
	}
	int	a1, a2, a3, a4;
	fscanf( pStream, "%d.%d.%d.%d:%d", &a1, &a2, &a3, &a4, &g_serverPort );
	g_serverAddr[0] = a1;
	g_serverAddr[1] = a2;
	g_serverAddr[2] = a3;
	g_serverAddr[3] = a4;
	printf( "Settings: server:%d.%d.%d.%d port:%d\n", g_serverAddr[0], g_serverAddr[1], g_serverAddr[2], g_serverAddr[3], g_serverPort );
	fclose( pStream );
	return true;
}


//////////////////////////////////////////////////////////////////////

//NOTE: kernel mode module flag and kernel mode thread are both required
PSP_MODULE_INFO("WIFI_TEST_APP", 0x1000, 1, 1); /* 0x1000 REQUIRED!!! */
PSP_MAIN_THREAD_ATTR(0); /* 0 REQUIRED!!! */
PSP_MAIN_THREAD_STACK_SIZE_KB(32); /* smaller stack for kernel thread */

int user_main(SceSize args, void* argp);

int main(int argc, char *argv[])
{
	// Kernel mode thread
	pspDebugScreenInit();

	strcpy( g_appPath, argv[0] );
	*strstr( g_appPath, "EBOOT.PBP" ) = 0;

	printf("PSP Controller v0.1\n");
	printf("by Mikko Mononen 2005\n");
	printf("\n");

	if (nlhLoadDrivers(&module_info) != 0)
	{
		printf("Driver load error\n");
		return 0;
	}

	// create user thread, tweek stack size here if necessary
	SceUID thid = sceKernelCreateThread("User Mode Thread", user_main,
		0x11, // default priority
		256 * 1024, // stack size (256KB is regular default)
		PSP_THREAD_ATTR_USER, NULL);

	// start user thread, then wait for it to do everything else
	sceKernelStartThread(thid, 0, NULL);
	sceKernelWaitThreadEnd(thid, NULL);

	sceKernelExitGame();
	return 0;
}

//////////////////////////////////////////////////////////////////////

static void ControllerLoop(const char* szMyIPAddr);

int user_main(SceSize args, void* argp)
{
	if( !LoadSettings() )
	{
		sceKernelDelayThread(4*1000000); // 4sec to read any final status
		return 0;
	}

	// user mode thread does all the real work
	u32 err;
	int connectionConfig = -1;
	int pick_count = 0;
	int stateLast = -1;

	// nlhInit must be called from user thread for DHCP to work
	err = nlhInit();
	if (err != 0)
	{
		printf("WiFi Init error\n");
		printf("nlhInit returns $%x\n", err);
		printf("Please check Network Settings\n");
		sceKernelDelayThread(10*1000000); // 10sec to read before exit
		goto close_net;
	}

	// enumerate connections
#define MAX_PICK 10
	struct
	{
		int index;
		char name[64];
	} picks[MAX_PICK];
	int iNetIndex;
	for (iNetIndex = 1; iNetIndex < 100; iNetIndex++) // skip the 0th connection
	{
		if (sceUtilityCheckNetParam(iNetIndex) != 0)
			break;  // no more
		sceUtilityGetNetParam(iNetIndex, 0, picks[pick_count].name);
		picks[pick_count].index = iNetIndex;
		pick_count++;
		if (pick_count >= MAX_PICK)
			break;  // no more room
	}

	if (pick_count == 0)
	{
		printf("No connections\n");
		printf("Please try Network Settings\n");
		sceKernelDelayThread(10*1000000); // 10sec to read before exit
		goto close_net;
	}

	printf("Found %d possible connections\n", pick_count);
#ifdef LATER
	if (pick_count > 1)
	{
		// customize user interface to let user pick
	}
#endif

	printf(" using the first one '%s'\n", picks[0].name);
	connectionConfig = picks[0].index;

	// Connect
	printf("using connection #%d\n", connectionConfig);
	err = sceNetApctlConnect(connectionConfig);
	if (err != 0)
	{
		printf("sceNetApctlConnect returns $%x\n", err);
		sceKernelDelayThread(10*1000000); // 10sec to read before exit
		goto close_net;
	}

	// Report status while waiting for connection to access point
	printf("Connecting...\n");
	while (1)
	{
		int state;
		err = sceNetApctlGetState(&state);
		if (err != 0)
		{
			printf("sceNetApctlGetState returns $%x\n", err);
			sceKernelDelayThread(10*1000000); // 10sec to read before exit
			goto close_connection;
		}
		if (state > stateLast)
		{
			printf("  connection state %d of 4\n", state);
			stateLast = state;
		}
		if (state == 4)
			break;  // connected with static IP

		// wait a little before polling again
		sceKernelDelayThread(50*1000); // 50ms
	}
	printf("Connected!\n");

	// connected, get my IPADDR and run test
	char szMyIPAddr[32];
	if (sceNetApctlGetInfo(8, szMyIPAddr) != 0)
		strcpy(szMyIPAddr, "unknown IP address");
	printf("my IPADDR = %s\n", szMyIPAddr);

	ControllerLoop(szMyIPAddr);

	// all done
close_connection:
	err = sceNetApctlDisconnect();
	if (err != 0)
		printf("sceNetApctlDisconnect returns $%x\n", err);

close_net:

	nlhTerm();

	printf("Program exiting\n");
	sceKernelDelayThread(4*1000000); // 4sec to read any final status

	return 0;
}

//////////////////////////////////////////////////////////////////////

static void ControllerLoop(const char* szMyIPAddr)
{
	// Open UDP socket to send the data to the host.
	SOCKET	sock = sceNetInetSocket(AF_INET, SOCK_DGRAM, 0);
	if (sock & 0x80000000)
	{
    printf("Could not connect.");
		return;
	}

	sockaddr_in addrTo;
  addrTo.sin_family = AF_INET;
  addrTo.sin_port = htons(g_serverPort);
  addrTo.sin_addr[0] = g_serverAddr[0];
  addrTo.sin_addr[1] = g_serverAddr[1];
  addrTo.sin_addr[2] = g_serverAddr[2];
  addrTo.sin_addr[3] = g_serverAddr[3];

	sceCtrlSetSamplingCycle( 0 );
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );

	SceCtrlData		pad;
	unsigned int packetNum = 1;
	unsigned char	packet[11];
	memset( packet, 0, sizeof( packet ) );

	printf( "\n\n" );

	while( true )
	{
		sceCtrlReadBufferPositive( &pad, 1 );

		if( pad.Buttons & PSP_CTRL_START )
			break;

		unsigned char	packet[11];
		packet[0] = '!';
		packet[1] = 'K';
		packet[2] = (packetNum) & 0xff;
		packet[3] = (packetNum >> 8) & 0xff;
		packet[4] = (packetNum >> 16) & 0xff;
		packet[5] = (packetNum >> 24) & 0xff;
		packet[6] = pad.Lx;
		packet[7] = pad.Ly;
		packet[8] = (pad.Buttons) & 0xff;
		packet[9] = (pad.Buttons >> 8) & 0xff;
		packet[10] = (pad.Buttons >> 16) & 0xff;
		packetNum++;

		sceNetInetSendto( sock, packet, sizeof( packet ), 0, &addrTo, sizeof(addrTo) );

		int	y = pspDebugScreenGetY();
		pspDebugScreenSetXY( 0, y );
		printf( "Lx:%03d Ly:%03d But:0x%08x", pad.Lx, pad.Ly, pad.Buttons );

		sceDisplayWaitVblankStart();
	}

	sceNetInetClose(sock);
}
