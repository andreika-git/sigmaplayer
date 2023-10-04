//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - debug messaging functions header file
 *  \file       sp_msg.h
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

#ifndef SP_MSG_H
#define SP_MSG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MSG_OUTPUT
{
	MSG_OUTPUT_HIDE = 0,
	MSG_OUTPUT_SHOW = 1,
	MSG_OUTPUT_FREEZE = 2,
} MSG_OUTPUT;

typedef enum MSG_FILTER
{
	MSG_FILTER_NONE = 0,

	MSG_FILTER_CRITICAL = 1,
	MSG_FILTER_ERROR = 2,
	MSG_FILTER_DEBUG = 4,

	MSG_FILTER_ALL = MSG_FILTER_CRITICAL | MSG_FILTER_ERROR | MSG_FILTER_DEBUG,
} MSG_FILTER;

/// Initialize debug/messaging system
void msg_init(void);

/// Set messages output type (show/hide/disable)
void msg_set_output(MSG_OUTPUT o);
/// Get messages output type
MSG_OUTPUT msg_get_output(void);

/// Set messages filter (see MSG_FILTER bitfields)
void msg_set_filter(int);
/// Get messages filter
int msg_get_filter(void);

/// Output debug message to the console.
void msg(const char *text, ...);
/// Output error message to the console.
void msg_error(const char *text, ...);
/// Output critical error message to the console.
void msg_critical(const char *text, ...);

/// Output system information
void msg_sysinfo(void);

/// Open TTY shell
void msg_shell(void);

#ifdef __cplusplus
}
#endif

#endif // of SP_MSG_H
