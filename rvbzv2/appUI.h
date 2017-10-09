/*
 * appUI.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */


#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>


#define COLUMN_1		2
#define COLUMN_2		41

#define LINE_DEVICES	2
#define LINE_MENUS		9
#define LINE_PSU		17
#define LINE_INFO		19

#define COLOR_DEVICES	ESC_FG_GREEN
#define COLOR_MENUS		ESC_FG_CYAN
#define COLOR_PSU		ESC_FG_RED
#define COLOR_INFO		ESC_FG_MAGENTA


typedef enum
{
	APPUI_DEVICES,
	APPUI_MENUS,
	APPUI_PSU,
	APPUI_INFO,
} APPUI_t;

/* EASY PROGMEM VARADIC PRINTF MACRO */
#define print(_str, ...)							printf_P(PSTR(_str), ##__VA_ARGS__);
#define println(_str, ...)							print("\r\n\33[0K" _str, ##__VA_ARGS__);
#define printpos(_line, _column, _str, ...)			print("\33[%u;%uH\33[0K" _str, _line, _column, ##__VA_ARGS__);

#define fprint(_file, _str, ...)					fprintf_P(_file, PSTR(_str), ##__VA_ARGS__); //fprintf(_file, PSTR(_str), ##__VA_ARGS__)
#define fprintln(_file, _str, ...)					fprint(_file, "\r\n\33[0K" _str, ##__VA_ARGS__);
#define fprintpos(_file, _line, _column, _str, ...)	fprint(_file, "\33[%u;%uH\33[0K" _str, _line, _column, ##__VA_ARGS__);

#define ANSI_CURSOR_HIDE							"\33[?25l"
#define ANSI_CURSOR_SHOW							"\33[?25l"
#define ANSI_ERASE_REST_OF_LINE						"\33[0K"


// #define appUIPrint(_str_P, ...)						print(_str_P, ##__VA_ARGS__);
// #define appUIPrintln(_str_P, ...)					println(_str_P, ##__VA_ARGS__);
// #define appUIPrintPos(_line, _column, _str_P, ...)	printpos(_line, _column, _str_P, ##__VA_ARGS__);

#define appUIPrintWebsite(_str_P, ...)				appUIPrintWebsite_P(PSTR(_str_P), ##__VA_ARGS__);
#define appUIPrint(_str_P, ...)						appUIPrint_P(PSTR(_str_P), ##__VA_ARGS__);
#define appUIPrintln(_str_P, ...)					appUIPrintln_P(PSTR(_str_P), ##__VA_ARGS__);
#define appUIPrintPos(_line, _column, _str_P, ...)	appUIPrintPos_P((_line), (_column), PSTR(_str_P), ##__VA_ARGS__);

void appUISetUI(APPUI_t UI);
void appUISetHTMLFont(APPUI_t UI);

void appUIClean();
void appUICleanUSB();
void appUICleanWebsite();

void appUIPrintWebsite_P(const char *str_P, ...);
void appUIvPrintWebsite_P(const char *str_P, va_list args);

void appUIPrint_P(const char *str_P, ...);
void appUIPrintln_P(const char *str_P, ...);
void appUIPrintPos_P(const uint8_t line, const uint8_t column, const char *str_P, ...);

char * appUIGetBufWebsite();
uint16_t appUIGetBufWebsiteLength();
size_t appUIGetBufWebsiteSpaceRemaining();
