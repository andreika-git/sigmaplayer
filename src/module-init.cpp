//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Main program module interface impl.
 *  \file       module-init.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       10.12.2008
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

// For main program, use "if NOT defined"!
#ifndef COMPILE_MODULE
	#define MODULE_SERVER
#endif

#ifdef MODULE_SERVER
	#define __USE_LARGEFILE64
	#define __LARGE64_FILES
	#define _LARGEFILE64_SOURCE
	#include <stdio.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <signal.h>
	#include <mntent.h>
	#include <errno.h>
	#include <string.h>
	#include <strings.h>
	#include <pwd.h>
	#include <setjmp.h>
	#include <sys/ioctl.h>
	#include <sys/time.h>
	#include <sys/uio.h>

	#include <libsp/sp_misc.h>
	#include <libsp/sp_msg.h>
	#include <libsp/sp_khwl.h>
	#include <libsp/sp_fip.h>
	#include <libsp/sp_video.h>
	#include <libsp/sp_cdrom.h>
	#include <libsp/sp_mpeg.h>

	#include <script.h>
	#include <media.h>
	#include <settings.h>

extern "C" { extern int __sigjmp_save (sigjmp_buf env, int savemask); };
#endif

#include <libsp/sp_module.h>
#include <module.h>


extern "C" {

MODULE_DECL_START()

	MODULE_DECL(script_error_callback),
	MODULE_DECL(script_player_saved_callback),
	MODULE_DECL(script_video_info_callback),
	MODULE_DECL(script_zoom_scroll_reset_callback),
	MODULE_DECL(script_speed_callback),
	MODULE_DECL(script_time_callback),
	MODULE_DECL(script_dvd_chapter_callback),
	MODULE_DECL(script_dvd_title_callback),
	MODULE_DECL(script_dvd_menu_callback),
	MODULE_DECL(script_totaltime_callback),
	MODULE_DECL(script_audio_lang_callback),
	MODULE_DECL(script_audio_stream_callback),
	MODULE_DECL(script_audio_info_callback),
	MODULE_DECL(script_spu_lang_callback),
	MODULE_DECL(script_spu_stream_callback),
	MODULE_DECL(script_framerate_callback),
	MODULE_DECL(script_framesize_callback),
	MODULE_DECL(script_update_variable),
	
	MODULE_DECL(mpeg_init),
	MODULE_DECL(mpeg_deinit),
	MODULE_DECL(mpeg_getaudiostream),
	MODULE_DECL(mpeg_getspustream),
	MODULE_DECL(mpeg_zoom_hor),
	MODULE_DECL(mpeg_zoom_ver),
	MODULE_DECL(mpeg_scroll),
	MODULE_DECL(mpeg_getbufbase),
	MODULE_DECL(mpeg_setaudioparams),
	MODULE_DECL(mpeg_getpacketlength),
	MODULE_DECL(mpeg_setbuffer),
	MODULE_DECL(mpeg_getframesize),
	MODULE_DECL(mpeg_setspeed),
	MODULE_DECL(mpeg_zoom_reset),
	MODULE_DECL(mpeg_is_displayed),
	MODULE_DECL(mpeg_parse_ac3_header),
	MODULE_DECL(mpeg_getaudioparams),
	MODULE_DECL(mpeg_wait),
	MODULE_DECL(mpeg_getrate),
	MODULE_DECL(mpeg_is_playing),
	MODULE_DECL(mpeg_get_video_format),
	MODULE_DECL(mpeg_setpts),
	MODULE_DECL(mpeg_resetstreams),
	MODULE_DECL(mpeg_setaudiostream),
	MODULE_DECL(mpeg_start),
	MODULE_DECL(mpeg_play),
	MODULE_DECL(mpeg_feed),
	MODULE_DECL(mpeg_setbufidx),
	MODULE_DECL(mpeg_setspustream),
	MODULE_DECL(mpeg_get_fps),
	MODULE_DECL(mpeg_audio_format_changed),
	MODULE_DECL(mpeg_extractpacket),
	MODULE_DECL(mpeg_feed_getlast),
	MODULE_DECL(mpeg_parse_program_stream_pack_header),
	MODULE_DECL(mpeg_findpacket),
	
	MODULE_DECL(khwl_display_clear),
	MODULE_DECL(khwl_stop),
	MODULE_DECL(khwl_setvideomode),
	MODULE_DECL(khwl_setproperty),
	MODULE_DECL(khwl_getproperty),
	MODULE_DECL(khwl_transformcoord),
	MODULE_DECL(khwl_set_window_zoom),
	
	MODULE_DECL(fip_write_string),
	MODULE_DECL(fip_write_special),
	MODULE_DECL(fip_clear),

	MODULE_DECL(msg_error),
	MODULE_DECL(msg),

	MODULE_DECL(settings_get),
	MODULE_DECL(settings_set),

	MODULE_DECL(media_open),
	MODULE_DECL(media_close),
	MODULE_DECL(media_free_block),
	MODULE_DECL(media_geterror),
	MODULE_DECL(media_get_next_block),
	MODULE_DECL(media_skip_buffer),

	MODULE_DECL(usleep),
	MODULE_DECL(gettimeofday),
	MODULE_DECL(closedir),
	MODULE_DECL(readdir),
	MODULE_DECL(opendir),

#ifndef WIN32
	MODULE_DECL(__errno_location),
    MODULE_DECL(__sigjmp_save),
    MODULE_DECL(atoi),
    MODULE_DECL(calloc),
    MODULE_DECL(chdir),
    MODULE_DECL(clock),
    MODULE_DECL(close),
    MODULE_DECL(fclose),
    MODULE_DECL(ferror),
    MODULE_DECL(fgets),
    MODULE_DECL(fopen64),
    MODULE_DECL(fprintf),
    MODULE_DECL(fread),
    MODULE_DECL(free),
    MODULE_DECL(fstat64),
    MODULE_DECL(getcwd),
    MODULE_DECL(getenv),
    MODULE_DECL(getmntent),
    MODULE_DECL(getpwuid),
    MODULE_DECL(getuid),
    MODULE_DECL(ioctl),
    MODULE_DECL(localtime),
    MODULE_DECL(lseek64),
    MODULE_DECL(malloc),
    MODULE_DECL(memset),
    MODULE_DECL(open64),
    MODULE_DECL(rand),
    MODULE_DECL(read),
    MODULE_DECL(readdir64),
    MODULE_DECL(readv),
    MODULE_DECL(realloc),
    MODULE_DECL(snprintf),
    MODULE_DECL(sprintf),
    MODULE_DECL(srand),
    MODULE_DECL(stat64),
    MODULE_DECL(strcasecmp),
    MODULE_DECL(strcat),
    MODULE_DECL(strdup),
    MODULE_DECL(strerror),
    MODULE_DECL(strftime),
    MODULE_DECL(strncasecmp),
    MODULE_DECL(strncat),
    MODULE_DECL(strncmp),
    MODULE_DECL(strncpy),
    MODULE_DECL(strtok),
    MODULE_DECL(strtol),
    MODULE_DECL(tolower),
    MODULE_DECL(toupper),
    MODULE_DECL(vsprintf),
	
	MODULE_DECL(fflush),
	MODULE_DECL(sigemptyset),
	MODULE_DECL(sigaddset),
	MODULE_DECL(sigprocmask),
	MODULE_DECL(sigfillset),
	MODULE_DECL(sigaction),
	MODULE_DECL(raise),
	
	MODULE_DECL(getpid),
	MODULE_DECL(kill),
#endif

MODULE_DECL_END();


MODULE_FUNC_TABLE init_funcs[] = 
{
	MODULE_ENTRY(script_error_callback),
	MODULE_ENTRY(script_player_saved_callback),
	MODULE_ENTRY(script_video_info_callback),
	MODULE_ENTRY(script_zoom_scroll_reset_callback),
	MODULE_ENTRY(script_speed_callback),
	MODULE_ENTRY(script_time_callback),
	MODULE_ENTRY(script_dvd_chapter_callback),
	MODULE_ENTRY(script_dvd_title_callback),
	MODULE_ENTRY(script_dvd_menu_callback),
	MODULE_ENTRY(script_totaltime_callback),
	MODULE_ENTRY(script_audio_lang_callback),
	MODULE_ENTRY(script_audio_stream_callback),
	MODULE_ENTRY(script_audio_info_callback),
	MODULE_ENTRY(script_spu_lang_callback),
	MODULE_ENTRY(script_spu_stream_callback),
	MODULE_ENTRY(script_framerate_callback),
	MODULE_ENTRY(script_framesize_callback),
	MODULE_ENTRY(script_update_variable),
	
	MODULE_ENTRY(mpeg_init),
	MODULE_ENTRY(mpeg_deinit),
	MODULE_ENTRY(mpeg_getaudiostream),
	MODULE_ENTRY(mpeg_getspustream),
	MODULE_ENTRY(mpeg_zoom_hor),
	MODULE_ENTRY(mpeg_zoom_ver),
	MODULE_ENTRY(mpeg_scroll),
	MODULE_ENTRY(mpeg_getbufbase),
	MODULE_ENTRY(mpeg_setaudioparams),
	MODULE_ENTRY(mpeg_getpacketlength),
	MODULE_ENTRY(mpeg_setbuffer),
	MODULE_ENTRY(mpeg_getframesize),
	MODULE_ENTRY(mpeg_setspeed),
	MODULE_ENTRY(mpeg_zoom_reset),
	MODULE_ENTRY(mpeg_is_displayed),
	MODULE_ENTRY(mpeg_parse_ac3_header),
	MODULE_ENTRY(mpeg_getaudioparams),
	MODULE_ENTRY(mpeg_wait),
	MODULE_ENTRY(mpeg_getrate),
	MODULE_ENTRY(mpeg_is_playing),
	MODULE_ENTRY(mpeg_get_video_format),
	MODULE_ENTRY(mpeg_setpts),
	MODULE_ENTRY(mpeg_resetstreams),
	MODULE_ENTRY(mpeg_setaudiostream),
	MODULE_ENTRY(mpeg_start),
	MODULE_ENTRY(mpeg_play),
	MODULE_ENTRY(mpeg_feed),
	MODULE_ENTRY(mpeg_setbufidx),
	MODULE_ENTRY(mpeg_setspustream),
	MODULE_ENTRY(mpeg_get_fps),
	MODULE_ENTRY(mpeg_audio_format_changed),
	MODULE_ENTRY(mpeg_extractpacket),
	MODULE_ENTRY(mpeg_feed_getlast),
	MODULE_ENTRY(mpeg_parse_program_stream_pack_header),
	MODULE_ENTRY(mpeg_findpacket),
	
	MODULE_ENTRY(khwl_display_clear),
	MODULE_ENTRY(khwl_stop),
	MODULE_ENTRY(khwl_setvideomode),
	MODULE_ENTRY(khwl_setproperty),
	MODULE_ENTRY(khwl_getproperty),
	MODULE_ENTRY(khwl_transformcoord),
	MODULE_ENTRY(khwl_set_window_zoom),
	
	MODULE_ENTRY(fip_write_string),
	MODULE_ENTRY(fip_write_special),
	MODULE_ENTRY(fip_clear),

	MODULE_ENTRY(msg_error),
	MODULE_ENTRY(msg),

	MODULE_ENTRY(settings_get),
	MODULE_ENTRY(settings_set),

	MODULE_ENTRY(media_open),
	MODULE_ENTRY(media_close),
	MODULE_ENTRY(media_free_block),
	MODULE_ENTRY(media_geterror),
	MODULE_ENTRY(media_get_next_block),
	MODULE_ENTRY(media_skip_buffer),

	MODULE_ENTRY(usleep),
	MODULE_ENTRY(gettimeofday),
	MODULE_ENTRY(closedir),
	MODULE_ENTRY(readdir),
	MODULE_ENTRY(opendir),

#ifndef WIN32
	MODULE_ENTRY(__errno_location),
    MODULE_ENTRY(__sigjmp_save),
    MODULE_ENTRY(atoi),
    MODULE_ENTRY(calloc),
    MODULE_ENTRY(chdir),
    MODULE_ENTRY(clock),
    MODULE_ENTRY(close),
    MODULE_ENTRY(fclose),
    MODULE_ENTRY(ferror),
    MODULE_ENTRY(fgets),
    MODULE_ENTRY(fopen64),
    MODULE_ENTRY(fprintf),
    MODULE_ENTRY(fread),
    MODULE_ENTRY(free),
    MODULE_ENTRY(fstat64),
    MODULE_ENTRY(getcwd),
    MODULE_ENTRY(getenv),
    MODULE_ENTRY(getmntent),
    MODULE_ENTRY(getpwuid),
    MODULE_ENTRY(getuid),
    MODULE_ENTRY(ioctl),
    MODULE_ENTRY(localtime),
    MODULE_ENTRY(lseek64),
    MODULE_ENTRY(malloc),
    MODULE_ENTRY(memset),
    MODULE_ENTRY(open64),
    MODULE_ENTRY(rand),
    MODULE_ENTRY(read),
    MODULE_ENTRY(readdir64),
    MODULE_ENTRY(readv),
    MODULE_ENTRY(realloc),
    MODULE_ENTRY(snprintf),
    MODULE_ENTRY(sprintf),
    MODULE_ENTRY(srand),
    MODULE_ENTRY(stat64),
    MODULE_ENTRY(strcasecmp),
    MODULE_ENTRY(strcat),
    MODULE_ENTRY(strdup),
    MODULE_ENTRY(strerror),
    MODULE_ENTRY(strftime),
    MODULE_ENTRY(strncasecmp),
    MODULE_ENTRY(strncat),
    MODULE_ENTRY(strncmp),
    MODULE_ENTRY(strncpy),
    MODULE_ENTRY(strtok),
    MODULE_ENTRY(strtol),
    MODULE_ENTRY(tolower),
    MODULE_ENTRY(toupper),
    MODULE_ENTRY(vsprintf),
	
	MODULE_ENTRY(fflush),
	MODULE_ENTRY(sigemptyset),
	MODULE_ENTRY(sigaddset),
	MODULE_ENTRY(sigprocmask),
	MODULE_ENTRY(sigfillset),
	MODULE_ENTRY(sigaction),
	MODULE_ENTRY(raise),

	MODULE_ENTRY(getpid),
	MODULE_ENTRY(kill),
#endif

	MODULE_ENTRY_END(),
};

}
