/* ANSI-Escape Codes
 * ANSI C Single-header file
 * public domain
 */

#ifndef ANSI_ESC_H
#define ANSI_ESC_H

#ifdef _WIN32
	#define ANSI_EXTENDED
#endif

#define ANSI_SET_CURSOR		"\033[%d;%dH"
#define ANSI_RESET_CURSOR	"\033[H"
#define ANSI_SAVE_CURSOR	"\033[s"
#define ANSI_RESTORE_CURSOR	"\033[u"

#ifdef ANSI_EXTENDED

	#define ANSI_CURSOR_UP		"\033[%dA"
	#define ANSI_CURSOR_DOWN	"\033[%dB"
	#define ANSI_CURSOR_RIGHT	"\033[%dC"
	#define ANSI_CURSOR_LEFT	"\033[%dD"

#endif

#define ANSI_CLEAR		"\033[2J"
#define ANSI_CLEAR_LINE		"\033[K"

#define ANSI_OFF		"\033[0m"
#define ANSI_BOLD		"\033[1m"
#define ANSI_UNDERSCORE		"\033[4m"
#define ANSI_BLINK		"\033[5m"
#define ANSI_REVERSE_VIDEO	"\033[7m"
#define ANSI_CONCEALED		"\033[8m"

#define ANSI_COLOR		"\033[%dm"

#define ANSI_FCOLOR_BLACK	"\033[30m"
#define ANSI_FCOLOR_RED		"\033[31m"
#define ANSI_FCOLOR_GREEN	"\033[32m"
#define ANSI_FCOLOR_YELLOW	"\033[33m"
#define ANSI_FCOLOR_BLUE	"\033[34m"
#define ANSI_FCOLOR_MAGENTA	"\033[35m"
#define ANSI_FCOLOR_CYAN	"\033[36m"
#define ANSI_FCOLOR_WHITE	"\033[37m"

#define ANSI_BCOLOR_BLACK	"\033[40m"
#define ANSI_BCOLOR_RED		"\033[41m"
#define ANSI_BCOLOR_GREEN	"\033[42m"
#define ANSI_BCOLOR_YELLOW	"\033[43m"
#define ANSI_BCOLOR_BLUE	"\033[44m"
#define ANSI_BCOLOR_MAGENTA	"\033[45m"
#define ANSI_BCOLOR_CYAN	"\033[46m"
#define ANSI_BCOLOR_WHITE	"\033[47m"

#define ANSI_COLOR_CLEAR    "\033[0m"

#endif /* ANSI_ESC_H */
