//
// PSP Controller PC host
//		by Mikko Mononen 2005 <memon@inside.org>
//
// Description:
//		This program receives the status of the PSP pad sent at each screen update (60Hz).
//		The data is received as 16 byte packed via UDP. This program was designed to
//		let the PSP act as controller to create quick game prototypes on PC.
//
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <winioctl.h>
#include "pspcontroller.h"
#include "ppjioctl.h"


#define	NUM_ANALOG	2							// Number of analog values which we will provide
#define	NUM_DIGITAL	15						// Number of digital values which we will provide


#pragma pack(push,1)							// All fields in structure must be byte aligned.
typedef struct
{
 unsigned long	signature;				// Signature to identify packet to PPJoy IOCTL
 char			numAnalog;							// Num of analog values we pass
 long			analog[NUM_ANALOG];			// Analog values
 char			numDigital;							// Num of digital values we pass
 char			digital[NUM_DIGITAL];		// Digital values
}	JOYSTICK_STATE;
#pragma pack(pop)


int main( int argc, char **argv )
{
	printf( "PSP Controller test PC host\nMikko Mononen 2005\n\nPress Esc to quit.\n\n" );

	HANDLE					h;
	char						ch;
	JOYSTICK_STATE	joyState;
	DWORD	retSize, rc;
	char*	devName;
	int		port;

	devName= "\\\\.\\PPJoyIOCTL1";
	if( argc >= 2 )
		devName = argv[1];

	port = 20001;
	if( argc >= 3 )
		port = atoi( argv[2] );

	// Open a handle to the control device for the first virtual joystick.
	// Virtual joystick devices are names PPJoyIOCTL1 to PPJoyIOCTL16.
	h = CreateFile( devName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );

	/* Make sure we could open the device! */
	if( h == INVALID_HANDLE_VALUE )
	{
		printf( "CreateFile failed with error code %d trying to open %s device\n", GetLastError(), devName );
		return 1;
	}

	// Initialise the IOCTL data structure
	joyState.signature = JOYSTICK_STATE_V1;
	joyState.numAnalog = NUM_ANALOG;
	joyState.numDigital = NUM_DIGITAL;

	if( !PSPCtrlInit( port ) )
	{
		printf( "Error initialising the PSPCtrl.\n" );
		return 1;
	}

	PSPCtrlData	ctrl;

	while( true )
	{
		if( _kbhit() )
		{
			if( getch() == 27 )
				break;
		}

		joyState.analog[0] = joyState.analog[1] = (PPJOY_AXIS_MIN + PPJOY_AXIS_MAX) / 2;
		memset( joyState.digital, 0, sizeof( joyState.digital ) );

		PSPCtrlRead( ctrl );

		int	x = (int)ctrl.Lx - 128;
		int	y = (int)ctrl.Ly - 128;

		// There is a deadzone at the center of the controller. The readins at that area are not reliable.
		const int PSP_DEAD_ZONE = 14;
		const int	PSP_RANGE = 127 - PSP_DEAD_ZONE;

		if( x <= -PSP_DEAD_ZONE )			x += PSP_DEAD_ZONE;
		else if( x >= PSP_DEAD_ZONE )	x -= PSP_DEAD_ZONE;
		else x = 0;
		if( x < -PSP_RANGE ) x = -PSP_RANGE;

		if( y <= -PSP_DEAD_ZONE )			y += PSP_DEAD_ZONE;
		else if( y >= PSP_DEAD_ZONE )	y -= PSP_DEAD_ZONE;
		else y = 0;
		if( y < -PSP_RANGE ) y = -PSP_RANGE;

		joyState.analog[0] = PPJOY_AXIS_MIN + (x + PSP_RANGE) * (PPJOY_AXIS_MAX - PPJOY_AXIS_MIN) / (2 * PSP_RANGE);
		joyState.analog[1] = PPJOY_AXIS_MIN + (y + PSP_RANGE) * (PPJOY_AXIS_MAX - PPJOY_AXIS_MIN) / (2 * PSP_RANGE);

		joyState.digital[0] = (ctrl.Buttons & PSP_CTRL_TRIANGLE) ? 1 : 0;
		joyState.digital[1] = (ctrl.Buttons & PSP_CTRL_CIRCLE) ? 1 : 0;
		joyState.digital[2] = (ctrl.Buttons & PSP_CTRL_CROSS) ? 1 : 0;
		joyState.digital[3] = (ctrl.Buttons & PSP_CTRL_SQUARE) ? 1 : 0;

		joyState.digital[4] = (ctrl.Buttons & PSP_CTRL_LTRIGGER) ? 1 : 0;
		joyState.digital[5] = (ctrl.Buttons & PSP_CTRL_RTRIGGER) ? 1 : 0;

		joyState.digital[6] = (ctrl.Buttons & PSP_CTRL_UP) ? 1 : 0;
		joyState.digital[7] = (ctrl.Buttons & PSP_CTRL_RIGHT) ? 1 : 0;
		joyState.digital[8] = (ctrl.Buttons & PSP_CTRL_DOWN) ? 1 : 0;
		joyState.digital[9] = (ctrl.Buttons & PSP_CTRL_LEFT) ? 1 : 0;

		joyState.digital[10] = (ctrl.Buttons & PSP_CTRL_SELECT) ? 1 : 0;
		joyState.digital[11] = (ctrl.Buttons & PSP_CTRL_START) ? 1 : 0;
		joyState.digital[12] = (ctrl.Buttons & PSP_CTRL_HOME) ? 1 : 0;
		joyState.digital[13] = (ctrl.Buttons & PSP_CTRL_HOLD) ? 1 : 0;
		joyState.digital[14] = (ctrl.Buttons & PSP_CTRL_NOTE) ? 1 : 0;

		// Send request to PPJoy for processing.
		// Currently there is no Return Code from PPJoy, this may be added at a
		// later stage. So we pass a 0 byte output buffer.
		if( !DeviceIoControl( h, IOCTL_PPORTJOY_SET_STATE, &joyState, sizeof( joyState ), NULL, 0, &retSize, NULL ) )
		{
			rc = GetLastError();
			if( rc == 2 )
			{
				printf( "Underlying joystick device deleted. Exiting read loop\n" );
				break;
			}
			printf( "DeviceIoControl error %d\n",rc );
		}

		Sleep( 10 );
	}	

	PSPCtrlEnd();
	CloseHandle( h );

	return 0;
}