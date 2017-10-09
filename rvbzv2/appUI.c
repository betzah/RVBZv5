/*
 * appUI.c
 *
 * Created: 1/1/2015 0:0:0 AM
 Author: Mfadl
 */

#include "appUI.h"
#include "peripherals.h"


typedef struct
{
	char ethWebsite[600];
	uint16_t ethWebsitePos;
	
	uint8_t line, column;
} appUI_t;

//static size_t appUIGetBufWebsiteSpaceRemaining();

static appUI_t appUI;

#define HEADER_OFFSET	0x36
#define WEBSITE_STR_START			PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<font>")


void appUISetUI(APPUI_t UI)
{
	appUISetHTMLFont(UI);
	
	switch (UI)
	{
		case APPUI_DEVICES:	print(ESC_CURSOR_POS_SAVE		COLOR_DEVICES);		break;
		case APPUI_MENUS:	print(ESC_CURSOR_POS_SAVE		COLOR_MENUS);		break;
		case APPUI_PSU:		print(ESC_CURSOR_POS_SAVE		COLOR_PSU);			break;
		case APPUI_INFO:	print(ESC_CURSOR_POS_RESTORE	COLOR_INFO);		break;
	}
}

void appUISetHTMLFont(APPUI_t UI)
{
	static bool firstCall = false;
	
	if (!firstCall)
		firstCall = true;
	else
		appUIPrintWebsite("</font>");
		
	switch (UI)
	{
		case APPUI_DEVICES:	appUIPrintWebsite("<font color=\"green\">");		break;
		case APPUI_MENUS:	appUIPrintWebsite("<font color=\"purple\">");		break;
		case APPUI_PSU:		appUIPrintWebsite("<font color=\"red\">");		break;
		case APPUI_INFO:	appUIPrintWebsite("<font color=\"magenta\">");	break;
	}
}

//*****************************************************************************

void appUIClean()
{
	appUICleanUSB();
	appUICleanWebsite();
}


void appUICleanUSB()
{
	print(ESC_RESET ESC_ERASE_DISPLAY ESC_BOLD_ON ESC_ITALICS_ON ANSI_CURSOR_HIDE);
	printpos(LINE_INFO, COLUMN_1, "");
}


void appUICleanWebsite()
{
	memset(&appUI.ethWebsite[0], 0, sizeof appUI.ethWebsite);
	strcpy_P(&appUI.ethWebsite[HEADER_OFFSET], WEBSITE_STR_START);
	appUI.ethWebsitePos = HEADER_OFFSET + strlen_P(WEBSITE_STR_START); // see tcp/ip stack code
	appUI.line = 0;
	appUI.column = 0;
}


//*****************************************************************************

void appUIvPrintWebsite_P(const char *str_P, va_list args)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		size_t charRemaining;
		if ((charRemaining = appUIGetBufWebsiteSpaceRemaining()))
		{
			appUI.ethWebsitePos += vsnprintf_P(appUI.ethWebsite + appUI.ethWebsitePos, charRemaining, str_P, args);
		}
	}
}


void appUIPrintWebsite_P(const char *str_P, ...)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		va_list args;			// is this allowed & safe?? nested va_list :-)
		va_start (args, str_P);
		appUIvPrintWebsite_P(str_P, args);
		va_end (args);
	}
}

//*****************************************************************************

void appUIPrint_P(const char *str_P, ...)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		va_list args;
		va_start (args, str_P);
		vfprintf_P(stdout, str_P, args);
		appUIvPrintWebsite_P(str_P, args);
		va_end (args);
	}
}


void appUIPrintln_P(const char *str_P, ...)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		printf_P(PSTR("\r\n"));
		appUIPrintWebsite_P(PSTR("<br>"));
		
		va_list args;
		va_start (args, str_P);
		vfprintf_P(stdout, str_P, args);
		appUIvPrintWebsite_P(str_P, args);
		va_end (args);
	}
}


void appUIPrintPos_P(const uint8_t line, const uint8_t column, const char *str_P, ...)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (line != appUI.line || column != appUI.column)
		{
			printf_P(PSTR("\33[%u;%uH\33[0K"), line, column);
			appUIPrintWebsite_P(PSTR("<br>"));
			appUI.line = line;
			appUI.column = column;
		}
		
		va_list args;
		va_start (args, str_P);
		vfprintf_P(stdout, str_P, args);
		appUIvPrintWebsite_P(str_P, args);
		va_end (args);
	}
}

//*****************************************************************************

char * appUIGetBufWebsite()
{
	return appUI.ethWebsite;
}


uint16_t appUIGetBufWebsiteLength()
{
	return appUI.ethWebsitePos - HEADER_OFFSET;
}


size_t appUIGetBufWebsiteSpaceRemaining()
{
	int16_t space = ((sizeof appUI.ethWebsite) - 1) - appUI.ethWebsitePos;

	if (space <= 0)
		return 0;
	else
		return (size_t)space;
}
