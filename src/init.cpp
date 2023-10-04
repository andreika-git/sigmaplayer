//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - main source file
 *  \file       init.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       4.07.2004
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

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_cdrom.h>

#include <mmsl/mmsl.h>
#include <script.h>
#include <gui/console.h>
#include <gui/image.h>

#include <settings.h>
#include <player.h>

//#define SHOW_CONSOLE_AT_STARTUP


#ifdef WIN32
//#define SHOW_CONSOLE_AT_STARTUP
#endif

bool is_sleeping = false, do_sleep = false, need_to_stop = false;
int steps = 0;

void gui_update()
{
	script_update(true);
	gui.Update();
}

int cycle(bool gfx_only)
{
	int but = fip_read_button(FALSE);
	if (but != 0)
	{
		if (but == -1)	// for win32 exit
			return -1;

		if (but == FIP_KEY_POWER)
		{
			is_sleeping = !is_sleeping;
			do_sleep = true;
			if (is_sleeping)
			{
				 if (!stop_all())
					 need_to_stop = true;
			}
		}

		else if (!is_sleeping)
		{
			if (but == FIP_KEY_PN)
			{
				if (msg_get_output() == MSG_OUTPUT_SHOW)
				{
					msg_set_output(MSG_OUTPUT_FREEZE);
				}
				else if (msg_get_output() == MSG_OUTPUT_FREEZE)
				{
					msg_set_output(MSG_OUTPUT_HIDE);
				} 
				else if (msg_get_output() == MSG_OUTPUT_HIDE)
				{
					msg_set_output(MSG_OUTPUT_SHOW);
				}
			}
			// PBC shows sysinfo in console mode
			else if (but == FIP_KEY_PBC)
			{
				if (msg_get_output() == MSG_OUTPUT_SHOW)
				{
#if 0
					SPMemoryManager &memManager = SPMemoryManager::GetHandle();
					memManager.TEST_DUMP();

					guiimg->DumpImages();
#endif
					msg_sysinfo();
				}
				else
					msg_shell();
			}
			script_key_callback(but);
		}
	}

	if (do_sleep)
	{
		if (!need_to_stop || stop_all())
		{
			msg("sleeping = %s\n", is_sleeping ? "true" : "false");
			player_turn_onoff(!is_sleeping);
			
			if (!is_sleeping)
			{
				disc_changed(CDROM_STATUS_UNKNOWN);
				script_skiptime();
			}
			need_to_stop = false;
			do_sleep = false;
		}
	}

	if (!is_sleeping)
	{
		script_update(gfx_only);

		if (mmsl != NULL)
		{
			if (!mmsl->Run())
			{
				if (!is_internal_playing())
					usleep(10000);
			}
		}

		if (!gui.Update())
		{
			// win32 exit
			return -1;
		}

		if (!is_playing())
		{
			if ((steps++) % 15 == 0)	// every 750 ms
				disc_changed();
		}
	} else
		usleep(50000);

	return 0;
}

int main(int /*argc*/, char * /*argv*/[])
{
    if (!fip_init(TRUE))
    	exit(1);

    // needed for some drives which do not eject at startup
    if (fip_read_button(FALSE) == FIP_KEY_FRONT_EJECT)
    {
	    if (!cdrom_init())
    		exit(4);
		cdrom_eject(TRUE);	// open tray
		usleep(2000000);
   	}

	fip_write_string("LoAd");

    if (!khwl_init(TRUE))
    	exit(2);

    if (!settings_init())
    	exit(3);

    if (!cdrom_init())
    	exit(4);

	if (!gui.Initialize())
		exit(5);

	console = new Console(60, 20);
	gui.AddWindow(console);
	gui.SwitchDisplay(true);

	msg_init();

#ifdef SHOW_CONSOLE_AT_STARTUP
	msg_set_output(MSG_OUTPUT_SHOW);
	msg("Starting debug (see sp_log.txt)...\n");
#endif

	if (msg_get_output() == MSG_OUTPUT_HIDE)
		gui.ShowWindow(console, false);

	script_init();

	disc_changed();

//debug_init();

	for (;;)
	{
		if (cycle() < 0)
			break;
	}

	// probably we won't get here in firmware...

	SPSafeDelete(mmsl);
	script_deinit();
	
	cdrom_deinit();
	khwl_deinit();
	fip_deinit();

	return 0;
}
