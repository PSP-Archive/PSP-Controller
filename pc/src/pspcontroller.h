
#ifndef __PSPCONTROLLER_H__
#define __PSPCONTROLLER_H__

enum PSPCtrlButtons
{
	PSP_CTRL_SELECT     = 0x000001,
	PSP_CTRL_START      = 0x000008,
	PSP_CTRL_UP         = 0x000010,
	PSP_CTRL_RIGHT      = 0x000020,
	PSP_CTRL_DOWN      	= 0x000040,
	PSP_CTRL_LEFT      	= 0x000080,
	PSP_CTRL_LTRIGGER   = 0x000100,
	PSP_CTRL_RTRIGGER   = 0x000200,
	PSP_CTRL_TRIANGLE   = 0x001000,
	PSP_CTRL_CIRCLE     = 0x002000,
	PSP_CTRL_CROSS      = 0x004000,
	PSP_CTRL_SQUARE     = 0x008000,
	PSP_CTRL_HOME       = 0x010000,
	PSP_CTRL_HOLD       = 0x020000,
	PSP_CTRL_NOTE       = 0x800000,
};

struct PSPCtrlData
{
	unsigned int		Buttons;
	unsigned char 	Lx;
	unsigned char 	Ly;
};

// Initialise the controller server
bool	PSPCtrlInit( int port );
void	PSPCtrlRead( PSPCtrlData& ctrl );
void	PSPCtrlEnd();


#endif