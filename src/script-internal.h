//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - script wrapper header file
 *  \file       script-internal.h
 *  \author     bombur
 *  \version    0.1
 *  \date       2.08.2004
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

#ifndef SP_SCRIPT_INTERNAL_H
#define SP_SCRIPT_INTERNAL_H

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_cdrom.h>
#include <libsp/sp_flash.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_video.h>

#include <gui/rect.h>
#include <gui/image.h>
#include <gui/text.h>
#include <gui/console.h>

#include <mmsl/mmsl.h>

#include <settings.h>
#include <cdda.h>
#include <audio.h>
#include <dvd.h>
#include <video.h>
#include <player.h>
#include <subtitle.h>

#include <script.h>

#include "script-vars.h"
#include "script-objs.h"

enum SCRIPT_ERRORS
{
	SCRIPT_ERRORS_ALL = 0,
	SCRIPT_ERRORS_GENERAL,
	SCRIPT_ERRORS_CRITICAL,
	SCRIPT_ERRORS_NONE,
};

enum PLAYER_REPEAT
{
	PLAYER_REPEAT_NONE = 0,
	PLAYER_REPEAT_SELECTION,
	PLAYER_REPEAT_TRACK,
	PLAYER_REPEAT_ALL,
	PLAYER_REPEAT_RANDOM,
};

enum ITEMS_SORT
{
	ITEMS_SORT_NONE = 0,
	ITEMS_SORT_NORMAL,
	ITEMS_SORT_INVERSE,
	ITEMS_SORT_RANDOM,
};

enum ITEM_TYPE
{
	ITEM_TYPE_NONE = 0,
	ITEM_TYPE_FILE,
	ITEM_TYPE_TRACK,
	ITEM_TYPE_FOLDER,
	ITEM_TYPE_UP,
	ITEM_TYPE_DVD,

	ITEM_TYPE_MAX,
};

enum PLAYER_TYPE
{
	PLAYER_TYPE_UNKNOWN = 0,
	PLAYER_TYPE_DVD,
	PLAYER_TYPE_AUDIOCD,
	PLAYER_TYPE_VIDEO,
	PLAYER_TYPE_AUDIO,
	PLAYER_TYPE_FILE,
	PLAYER_TYPE_FOLDER,
};

enum PLAYER_COLOR_SPACE
{
	PLAYER_COLOR_SPACE_UNKNOWN = 0,
	PLAYER_COLOR_SPACE_GRAYSCALE = 1,
	PLAYER_COLOR_SPACE_YCRCB = 3,
};

const int num_file_masks = 5;

///////////////////////////////////////////

class StringPair
{
public:
	const char *str;
	int value;

	static int Get(MmslVariable *var, const StringPair *arr, int def)
	{
		SPString str = var->GetString();
		for (int i = 0; arr[i].str != NULL; i++)
		{
			if (str.CompareNoCase(arr[i].str) == 0)
				return arr[i].value;
		}
		return def;
	}

	static bool Set(MmslVariable *var, const StringPair *arr, int value)
	{
		for (int i = 0; arr[i].str != NULL; i++)
		{
			if (arr[i].value == value)
			{
				var->Set(arr[i].str);
				return true;
			}
		}
		return false;
	}
};

const int items_num_hash = 117;

class ItemsList;

/// List item
class Item
{
public:
	/// ctor
	Item()
	{
		type = ITEM_TYPE_NONE;
		mask_index = 0;
		oldidx = -1;
		parent = NULL;
		
		playlist = NULL;
		playlist_idx = -1;

		datetime = 0;
		size = 0;

		prev = NULL;
		next = NULL;
		
		namehash = 0;
	}

public:
	/// filename (file.ext) or item name
	SPString name;
	/// item type
	ITEM_TYPE type;
	/// mask index
	int mask_index;

	time_t datetime;
	LONGLONG size;
	
	// used to search new index after sorting
	int oldidx;

	// parent list index (primary only, not for play-lists)
	ItemsList *parent;

	/// last playlist the item was added to (if multiple)
	ItemsList *playlist;
	int playlist_idx;

public:
	// hash-related stuff...
	Item *prev, *next;
	DWORD namehash;

	/// compare
	template <typename T>
	bool operator == (const T & r)
	{
		if (name == NULL || r.name == NULL)
			return false;
		return name.CompareNoCase(r.name) == 0;
	}

	inline operator DWORD () const
	{
		return namehash;
	}

	const Item * const GetItem() const
	{
		return this;
	}

	/// Set variable name & calc. hash
	void SetName(const SPString & n)
	{
		name = n;
		namehash = SPStringHashFunc(name, items_num_hash);
	}

};

class ItemsList
{
public:
	/// ctor
	ItemsList()
	{
		cur = -1;
		copied = FALSE;
		filesize = 0;
		filetime = 0;
		mask_index = 0;

		for (int i = 0; i < ITEM_TYPE_MAX; i++)
			add4types[i] = 0;

		itemhash = NULL;

		random_idx = NULL;
		random_ridx = NULL;
		cur_random_pos = -1;
	}

	/// dtor
	~ItemsList()
	{
		// don't delete itemhash items!
		if (itemhash != NULL)
		{
			itemhash->DeleteObjects();
			delete itemhash;
			items.Clear();
		}
		else
			items.DeleteObjects();
		SPSafeDeleteArray(random_idx);
		SPSafeDeleteArray(random_ridx);
	}

	void Update();

public:
	SPList<Item *> items;
	// "/folder1/folder2/"
	SPString folder;
	SPString allmask, mask[num_file_masks];
	int cur;

	// some cached current data:
	SPString path, fname, ext;
	ITEM_TYPE type;
	BOOL copied;
	LONGLONG filesize;
	time_t filetime;
	int mask_index;

	int add4types[ITEM_TYPE_MAX];

	SPHashListAbstract<Item, Item> *itemhash;

	// random indexes
	int *random_idx;
	// reversed random indexes
	int *random_ridx;
	// random index
	int cur_random_pos;
};

class ItemInfo
{
public:
	/// ctor
	ItemInfo()
	{
		cur_time = 0;
		length = 0;
		
		cur_title = cur_chapter = 0;
		real_cur_title = real_cur_chapter = 0;
		real_cur_time = 0;
		cur_changed = false;
		cur_time_changed = false;
		num_titles = num_chapters = 0;
		width = height = 0;
		frame_rate = 0;
		clrs = PLAYER_COLOR_SPACE_UNKNOWN;

		dvd_menu_lang = "en";
		dvd_audio_lang = "en";
		spu_lang = "en";
		
		audio_stream = spu_stream = 0;

		subtitle_charset = SUBTITLE_CHARSET_DEFAULT;
		subtitle_wrap = 35;
	}

public:
	SPString name;
	SPString artist;
	SPString audio_info;
	SPString video_info;
	
	SPString subtitle;
	SUBTITLE_CHARSET subtitle_charset;
	int subtitle_wrap;
	
	int cur_time;
	int length;
	
	int cur_title, cur_chapter;
	int real_cur_title, real_cur_chapter, real_cur_time;
	bool cur_changed, cur_time_changed;
	int num_titles, num_chapters;

	int width, height;
	int frame_rate;
	// color space
	PLAYER_COLOR_SPACE clrs;

	SPString dvd_menu_lang;
	SPString dvd_audio_lang;
	SPString spu_lang;		// for dvd and video subtitles
	int audio_stream;
	int spu_stream;		// for dvd and video subtitles
};

/// Current script parameters
class ScriptParams
{
public:
	/// ctor
	ScriptParams()
	{
		errors = SCRIPT_ERRORS_ALL;
		status = CDROM_STATUS_UNKNOWN;
		require_next_status = CDROM_STATUS_UNKNOWN;
		saved_iso_status = CDROM_STATUS_UNKNOWN;
		key = 0;

		dvdplaying = FALSE;
		cddaplaying = FALSE;
		fileplaying = FALSE;
		videoplaying = FALSE;
		audioplaying = FALSE;
		waitinfo = FALSE;
		wasejected = FALSE;

		need_to_toggle_tray = FALSE;

		// screen:
		pal_idx = 0;
		hscale = 100; vscale = 100; rotate = 0;
		hscroll = 0; vscroll = 0;
		backleft = backtop = 0; backright = 719; backbottom = 479;
		bigbackleft = bigbacktop = 0; bigbackright = 719; bigbackbottom = 479;
		bigback_width = 0; bigback_height = 0;
		need_setwindow = false;

		// explorer:
		curitem = NULL;
		curtarget = NULL;
		lastitem = NULL;
		iso_lang = "iso8859-1";
		iso_lang_changed = true;
		filter[0] = ITEM_TYPE_UP;
		filter[1] = ITEM_TYPE_FOLDER;
		filter[2] = ITEM_TYPE_TRACK;
		filter[3] = ITEM_TYPE_DVD;
		filter[4] = ITEM_TYPE_FILE;
		filter[5] = ITEM_TYPE_NONE;
		sort = ITEMS_SORT_NONE;
		mask[0] = "*.*";
		for (int i = 1; i < num_file_masks; i++)
			mask[i] = "";
		allmask = mask[0];
		list_changed = false;

		// player:
		player_type = PLAYER_TYPE_UNKNOWN;
		player_do_command = false;
		player_debug = false;
		player_repeat = PLAYER_REPEAT_NONE;
		speed = MPEG_SPEED_STOP;

		// flash:
		flash_address = -1;
		flash_progress = -1;
		flash_p = -1;

		// timer support
		curtime = old_curtime = start_sec = 0;
	}

	/// dtor
	~ScriptParams()
	{
		lists.DeleteObjects();
		playlists.DeleteObjects();
		timed_objs.Delete();
	}

public:
	SCRIPT_ERRORS errors;

	SPString back, bigback;
	SPString pal;
	SPString font;
	int pal_idx;

	SPString fw_ver, mmsl_ver;

	CDROM_STATUS status;
	CDROM_STATUS require_next_status;
	CDROM_STATUS saved_iso_status;
	BOOL need_to_toggle_tray;

	int key;
	SPString keystr;

	int hscale, vscale, hscroll, vscroll, rotate;
	int backleft, backtop, backright, backbottom;
	int bigbackleft, bigbacktop, bigbackright, bigbackbottom;
	int bigback_width, bigback_height;
	bool need_setwindow;

	BOOL dvdplaying, fileplaying, videoplaying, audioplaying, cddaplaying, waitinfo;
	bool wasejected;

	// explorer

	SPString iso_lang;
	bool iso_lang_changed;
	
	SPString folder, allmask, mask[num_file_masks];
	SPString target;
	ITEM_TYPE filter[ITEM_TYPE_MAX];
	ITEMS_SORT sort;

	SPList<ItemsList *> lists;
	SPList<ItemsList *> playlists;
	ItemsList *curitem;
	ItemsList *curtarget;
	Item *lastitem;
	bool list_changed;

	// player

	SPString player_command;
	bool player_do_command;

	PLAYER_TYPE player_type;
	SPString player_source, player_folder;
	PLAYER_REPEAT player_repeat;
	ItemInfo info;
	SPString player_error;
	bool player_debug;
	// used for external players
	MPEG_SPEED_TYPE speed;

	// flash:
	SPString flash_file;
	int flash_address;
	int flash_progress, flash_p;

	/// timer support
	SPDLinkedListAbstract<ScriptTimerObject, ScriptTimerObject> timed_objs;
	int curtime, old_curtime, start_sec;
};

extern ScriptParams *params;
extern Item *tmpit;

void script_explorer_reset();

extern bool player_dvd_command(const SPString & command);
extern bool player_file_command(const SPString & command);
extern bool player_video_command(const SPString & command);
extern bool player_audio_command(const SPString & command);
extern bool player_cdda_command(const SPString & command);

extern void player_do_command();

#endif // of SP_SCRIPT_INTERNAL_H
