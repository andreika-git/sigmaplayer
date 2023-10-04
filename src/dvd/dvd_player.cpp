//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - DVD player wrapper impl.
 *  \file       dvd/dvd_player.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       12.10.2006
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
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <dirent.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_cdrom.h>
#include <libsp/sp_khwl.h>

#include <dvd.h>
#include "dvd-internal.h"

#include <script-internal.h>

bool dvd_do_command(const SPString & command)
{
	if (command.CompareNoCase("continue") == 0)
	{
		dvd_continue_play();
		dvd_button_play();
		script_update_variable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
  	else if (command.CompareNoCase("pause") == 0)
	{
		dvd_button_pause(); 
		script_update_variable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
	else if (command.CompareNoCase("step") == 0)
	{
		dvd_button_step(); 
		return true;
	}
	else if (command.CompareNoCase("slow") == 0)
	{
		dvd_button_slow(); 
		script_update_variable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
  	else if (command.CompareNoCase("forward") == 0)
	{
		dvd_button_fwd(); 
		script_update_variable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
  	else if (command.CompareNoCase("rewind") == 0)
	{
		dvd_button_rew(); 
		script_update_variable(SCRIPT_VAR_PLAYER_SPEED);
		return true;
	}
  	else if (command.CompareNoCase("prev") == 0)
	{
		dvd_button_prev(); return true;
	}
  	else if (command.CompareNoCase("next") == 0)
	{
		dvd_button_next(); return true;
	}
  	else if (command.CompareNoCase("press") == 0)
	{
		dvd_button_press(); return true;
	}
  	else if (command.CompareNoCase("left") == 0)
	{
		dvd_button_left(); return true;
	}
  	else if (command.CompareNoCase("right") == 0)
	{
		dvd_button_right(); return true;
	}
  	else if (command.CompareNoCase("up") == 0)
	{
		dvd_button_up(); return true;
	}
  	else if (command.CompareNoCase("down") == 0)
	{
		dvd_button_down(); return true;
	}
  	else if (command.CompareNoCase("menu") == 0)
	{
		dvd_button_menu(); return true;
	}
	else if (command.CompareNoCase("rootmenu") == 0)
	{
		dvd_button_menu(DVD_MENU_ROOT); return true;
	}
	else if (command.CompareNoCase("return") == 0)
	{
		dvd_button_return(); return true;
	}
  	else if (command.CompareNoCase("angle") == 0)
	{
		dvd_button_angle(); return true;
	}
  	else if (command.CompareNoCase("audio") == 0)
	{
		dvd_button_audio(); 
		script_update_variable(SCRIPT_VAR_PLAYER_LANGUAGE_AUDIO);
		script_update_variable(SCRIPT_VAR_PLAYER_AUDIO_STREAM);
		return true;
	}
  	else if (command.CompareNoCase("subtitle") == 0)
	{
		dvd_button_subtitle(); 
		script_update_variable(SCRIPT_VAR_PLAYER_LANGUAGE_SUBTITLE);
		script_update_variable(SCRIPT_VAR_PLAYER_SUBTITLE_STREAM);
		return true;
	}
	return false;
}
