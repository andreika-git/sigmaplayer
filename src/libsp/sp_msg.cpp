//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - debug messaging functions impl.
 *  \file       sp_msg.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       4.05.2004
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 */
//////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/sysinfo.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>

#ifndef SP_MSG_SIMPLE
#include <gui/window.h>
#include <gui/console.h>
#endif

// 0 = off, 1 = on, 2 = ready_to_clear
MSG_OUTPUT msg_output = MSG_OUTPUT_HIDE;
int msg_filter = MSG_FILTER_ALL;

static char msgbuf[4096];

void msg_set_filter(int f)
{
	msg_filter = f;
}

int msg_get_filter()
{
	return msg_filter;
}

void msg_init()
{
#ifndef SP_MSG_SIMPLE
#ifdef WIN32
	FILE *fp = fopen("sp_log.txt", "wt");
	fclose(fp);
#endif
#endif
}

void msg_set_output(MSG_OUTPUT o)
{
	if (o == MSG_OUTPUT_SHOW)
	{
		msg_output = o;
#ifndef SP_MSG_SIMPLE
		gui.UpdateEnable(true);
		gui.ShowWindow(console, true);
#endif
		msg("DEBUG show...\n");
	}
	else if (o == MSG_OUTPUT_HIDE)
	{
		msg_output = o;
#ifndef SP_MSG_SIMPLE
		gui.ShowWindow(console, false);
#endif
	}
	else if (o == MSG_OUTPUT_FREEZE)
	{
		msg("DEBUG stop...\n");
		msg_output = o;
	}
}

MSG_OUTPUT msg_get_output()
{
	return msg_output;
}

///////////////////////////////////////////////

static void msg_internal(const char *msgbuf)
{
	if (msg_output != MSG_OUTPUT_FREEZE)
	{
		printf(msgbuf);fflush(stdout);
	}

#ifndef SP_MSG_SIMPLE
	if (console != NULL)
		console->AddString(msgbuf);

#ifdef WIN32
	FILE *fp = fopen("sp_log.txt", "at");
	if (fp) { fprintf(fp, msgbuf);fclose(fp); }
#endif
#endif
}

void msg(const char *text, ...)
{
	if (msg_output != MSG_OUTPUT_FREEZE && (msg_filter & MSG_FILTER_DEBUG) == MSG_FILTER_DEBUG)
	{
		va_list args;
		va_start(args, text);
		vsnprintf(msgbuf, 4096, text, args);
		va_end(args);
		msg_internal(msgbuf);
	}
}

void msg_error(const char *text, ...)
{
	if (msg_output != MSG_OUTPUT_FREEZE && (msg_filter & MSG_FILTER_ERROR) == MSG_FILTER_ERROR)
	{
		va_list args;
		va_start(args, text);
		vsnprintf(msgbuf, 4096, text, args);
		va_end(args);
		msg_internal(msgbuf);
	}
}

void msg_critical(const char *text, ...)
{
	if (/*msg_output != MSG_OUTPUT_FREEZE && */ (msg_filter & MSG_FILTER_CRITICAL) == MSG_FILTER_CRITICAL)
	{
		va_list args;
		va_start(args, text);
		vsnprintf(msgbuf, 4096, text, args);
		va_end(args);
		msg_internal(msgbuf);
	}
}

void msg_sysinfo()
{
#ifndef SP_MSG_SIMPLE
	struct sysinfo si;
	sysinfo(&si);

	msg("uptime   : %d\n", si.uptime);
	msg("totalram   : %d\n", si.totalram);
	msg("freeram   : %d\n", si.freeram);
	//msg("sharedram   : %d\n", si.sharedram);
	//msg("bufferram   : %d\n", si.bufferram);
	//msg("totalswap   : %d\n", si.totalswap);
	//msg("freeswap   : %d\n", si.freeswap);
	msg("procs   : %d\n", si.procs);
	//msg("totalhigh   : %d\n", si.totalhigh);
	//msg("freehigh   : %d\n", si.freehigh);
	//msg("mem_unit   : %d\n", si.mem_unit);
#endif
}

void msg_shell()
{
#ifndef SP_MSG_SIMPLE
	static const char *args[] = { "busybox", "sh", NULL };
	printf("Starting SHELL...\n");
	exec_file("/bin/busybox", args);
#endif
}
