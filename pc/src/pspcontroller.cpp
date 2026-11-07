#include <stdio.h>
#include <winsock2.h>
#include "pspcontroller.h"

#pragma comment(lib, "ws2_32.lib ")

static HANDLE									g_serverThread = 0;
static SOCKET									g_serverSocket = INVALID_SOCKET;
static volatile unsigned char	g_CtrlLx = 0;
static volatile unsigned char	g_CtrlLy = 0;
static volatile unsigned int	g_CtrlButtons = 0;

#define LOG	printf

static DWORD GetTick()
{
	LARGE_INTEGER	d;
	double			i, j;
	QueryPerformanceCounter( &d );
	i = (double)d.QuadPart;
	QueryPerformanceFrequency( &d );
	j = (double)d.QuadPart;
	return (DWORD)((i / j) * 1000.0);
}

static DWORD WINAPI ServerThread( LPVOID pVal )
{
	unsigned int	lastPacket = 0;
	unsigned char	buf[256];
	sockaddr_in		addrFrom;
  int						sizeAddrFrom = sizeof( addrFrom );
	DWORD					lastTime = GetTick();

	// Listen to the specified port and parse the incoming data.
	while( true )
	{
		// Receive data from the specified port.
		int	n = recvfrom( g_serverSocket, (char*)buf, sizeof( buf ), 0, (sockaddr*)&addrFrom, &sizeAddrFrom );
		if( n )
		{
			DWORD	curTime = GetTick();

			// Parse the packet (assumes that the packet is not split!).
			int	i = 0;
			while( i < n )
			{
				if( (i + 11) <= n && buf[i] == '!' && buf[i+1] == 'K' )
				{
					// A piece of data starting with !K of size 11 bytes has bee recognised, it's ours!
					// Parse the packet number. Each send packet has an incrementing 32bit identifier. Discard old packets.
					unsigned int packetNum = 0;
					packetNum = buf[i+2];
					packetNum = buf[i+3] << 8;
					packetNum = buf[i+4] << 16;
					packetNum = buf[i+5] << 24;
					if( lastPacket != 0 && packetNum < lastPacket )
					{
						// The current packet is older than what we have already received, ignore it,
						LOG( "Got old packet %s (last=%d)\n", packetNum, lastPacket );
						i += 11;
						continue;
					}
					// Parse the actual controller info.
					g_CtrlLx = buf[i+6];
					g_CtrlLy = buf[i+7];
					g_CtrlButtons = buf[i+8];
					g_CtrlButtons |= buf[i+9] << 8;
					g_CtrlButtons |= buf[i+10] << 16;
					i += 11;	// there are some extra space here for no obvious reason.
					lastPacket = packetNum;
				}
				else
				{
					// Unrecognised character, skip.
					i++;
				}
			}
//			LOG( "Received(%04d): Lx=%03d Ly=%03d But=0x%06x  dt:%dms\n", n, g_CtrlLx, g_CtrlLy, g_CtrlButtons, curTime - lastTime );
			lastTime = curTime;
		}
	}	

	return 0;
}

// Initialise the 
bool	PSPCtrlInit( int port )
{
	PSPCtrlEnd();

	WSADATA				wsaData;
	if( WSAStartup( 0x101, &wsaData ) != 0 )
		return false;

	// Listen to UDP port
	sockaddr_in		local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons( (u_short)port );

	g_serverSocket = socket( AF_INET, SOCK_DGRAM, 0 );
	if( g_serverSocket == INVALID_SOCKET )
	{
		WSACleanup();
		return false;
	}

	if( bind( g_serverSocket, (sockaddr*)&local, sizeof( local ) ) != 0 )
	{
		WSACleanup();
		return false;
	}

	// Start the polling thread.
	DWORD	dwThreadId;
	g_serverThread = CreateThread( NULL, 0, ServerThread, 0, 0, &dwThreadId );
	if( g_serverThread == NULL )
		return false;

	return true;
}

void	PSPCtrlRead( PSPCtrlData& ctrl )
{
	// copy the last received controller info.
	ctrl.Lx = g_CtrlLx;
	ctrl.Ly = g_CtrlLy;
	ctrl.Buttons = g_CtrlButtons;
}

void	PSPCtrlEnd()
{
	// Kill the server
	if( g_serverThread )
	{
		TerminateThread( g_serverThread, 0 );
		g_serverThread = 0;
	}

	if( g_serverSocket != INVALID_SOCKET )
	{
		closesocket( g_serverSocket );
		g_serverSocket = INVALID_SOCKET; 
	}

	WSACleanup();
}
