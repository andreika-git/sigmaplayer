//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - DVD module interface impl.
 *  \file       module-dvd.cpp
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

#ifdef COMPILE_MODULE
	#define MODULE_SERVER
#endif

#ifdef MODULE_SERVER
	#include <stdio.h>
	#include <libsp/sp_misc.h>
	#include <libsp/sp_khwl.h>
	
	#include <dvd.h>
#endif

#include <libsp/sp_module.h>
#include <module.h>

extern "C" {

MODULE_DECL_START()

	MODULE_DECL(dvd_open),
	MODULE_DECL(dvd_get_next_block),
	MODULE_DECL(dvd_free_block),
	MODULE_DECL(dvd_close),
	MODULE_DECL(dvd_reset),
	MODULE_DECL(dvd_error_string),
	MODULE_DECL(dvd_do_command),
	MODULE_DECL(dvd_stop),
	MODULE_DECL(dvd_getnumtitles),
	MODULE_DECL(dvd_play),
	MODULE_DECL(dvd_setdeflang_spu),
	MODULE_DECL(dvd_setdeflang_audio),
	MODULE_DECL(dvd_setdeflang_menu),
	MODULE_DECL(dvd_button_play),
	MODULE_DECL(dvd_getnumchapters),
	MODULE_DECL(dvd_get_cur),
	MODULE_DECL(dvd_seek_titlepart),
	MODULE_DECL(dvd_seek),
	MODULE_DECL(dvd_getdebug),
	MODULE_DECL(dvd_ismenu),
	MODULE_DECL(dvd_getangle),
	MODULE_DECL(dvd_getspeed),
	MODULE_DECL(dvd_get_saved),
	MODULE_DECL(dvd_setdebug),
	MODULE_DECL(dvd_get_total_time),
	MODULE_DECL(dvd_get_chapter_for_time),
	MODULE_DECL(dvd_button_subtitle),
	MODULE_DECL(dvd_button_audio),
	MODULE_DECL(dvd_button_menu),
	MODULE_DECL(dvd_button_angle),
	MODULE_DECL(dvd_setspeed),
	MODULE_DECL(dvd_player_loop),

MODULE_DECL_END();


MODULE_FUNC_TABLE dvd_funcs[] = 
{
	MODULE_ENTRY(dvd_open),
	MODULE_ENTRY(dvd_get_next_block),
	MODULE_ENTRY(dvd_free_block),
	MODULE_ENTRY(dvd_close),
	MODULE_ENTRY(dvd_reset),
	MODULE_ENTRY(dvd_error_string),
	MODULE_ENTRY(dvd_do_command),
	MODULE_ENTRY(dvd_stop),
	MODULE_ENTRY(dvd_getnumtitles),
	MODULE_ENTRY(dvd_play),
	MODULE_ENTRY(dvd_setdeflang_spu),
	MODULE_ENTRY(dvd_setdeflang_audio),
	MODULE_ENTRY(dvd_setdeflang_menu),
	MODULE_ENTRY(dvd_button_play),
	MODULE_ENTRY(dvd_getnumchapters),
	MODULE_ENTRY(dvd_get_cur),
	MODULE_ENTRY(dvd_seek_titlepart),
	MODULE_ENTRY(dvd_seek),
	MODULE_ENTRY(dvd_getdebug),
	MODULE_ENTRY(dvd_ismenu),
	MODULE_ENTRY(dvd_getangle),
	MODULE_ENTRY(dvd_getspeed),
	MODULE_ENTRY(dvd_get_saved),
	MODULE_ENTRY(dvd_setdebug),
	MODULE_ENTRY(dvd_get_total_time),
	MODULE_ENTRY(dvd_get_chapter_for_time),
	MODULE_ENTRY(dvd_button_subtitle),
	MODULE_ENTRY(dvd_button_audio),
	MODULE_ENTRY(dvd_button_menu),
	MODULE_ENTRY(dvd_button_angle),
	MODULE_ENTRY(dvd_setspeed),
	MODULE_ENTRY(dvd_player_loop),
	
	MODULE_ENTRY_END(),
};

MODULE_DESC module_dvd_desc = 
{
	"dvd",					// name
	"dvd",					// class
	"DVD Player module",	// desc.
	100,					// version = 1.00
	140,					// min. init ver = 1.40

	{ 0 },
};

// end of extern "C"
}
