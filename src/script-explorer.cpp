//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - file explorer wrapper impl.
 *  \file       script-explorer.cpp
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
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <dirent.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_cdrom.h>
#include <libsp/sp_flash.h>
#include <libsp/sp_fip.h>
#include <libsp/sp_khwl.h>

#include <gui/rect.h>
#include <gui/image.h>
#include <gui/text.h>
#include <gui/console.h>
#include <mmsl/mmsl.h>

#include <settings.h>
#include <cdda.h>
#include <script.h>
#include <script-internal.h>


static const StringPair items_sort_pairs[] = 
{ 
	{ "none", ITEMS_SORT_NONE },
	{ "normal", ITEMS_SORT_NORMAL },
	{ "inverse", ITEMS_SORT_INVERSE },
	{ "random", ITEMS_SORT_RANDOM },
	{ NULL, -1 },
};

static const StringPair item_type_pairs[] = 
{ 
	{ "", ITEMS_SORT_NONE },
	{ "file", ITEM_TYPE_FILE },
	{ "track", ITEM_TYPE_TRACK },
	{ "folder", ITEM_TYPE_FOLDER },
	{ "up", ITEM_TYPE_UP },
	{ "dvd", ITEM_TYPE_DVD },
	{ NULL, -1 },
};

// used for items hash
Item *tmpit = NULL;

static int ItemsListCompareFuncNormal(Item **i1, Item **i2)
{
	return (*i1)->name.CompareNoCase((*i2)->name);
}
static int ItemsListCompareFuncReverse(Item **i1, Item **i2)
{
	return -(*i1)->name.CompareNoCase((*i2)->name);
}


////////////////////////////////////////////////////////////////////////

void ItemsList::Update()
{
	if (cur >= 0 && cur < items.GetN())
	{
		// get extension
		int eidx = items[cur]->name.ReverseFind('.');
		if (eidx < 0) 
			eidx = items[cur]->name.GetLength();
		fname = items[cur]->name.Left(eidx);
		if (fname.FindNoCase("/cdrom/") == 0)
			fname = fname.Mid(7);
		if (fname.FindNoCase("/hdd/") == 0)
			fname = fname.Mid(5);
		if (fname.FindNoCase("/") == 0)
			fname = fname.Mid(1);
		ext = items[cur]->name.Mid(eidx);
		if (itemhash != NULL)	// playlist
			path = items[cur]->name;
		else
			path = folder + items[cur]->name;
		type = items[cur]->type;
		filetime = items[cur]->datetime;
		mask_index = items[cur]->mask_index;
		filesize = MAX(items[cur]->size, 0);

		params->lastitem = items[cur];

		if (params->curtarget != NULL && params->curtarget->itemhash != NULL)
		{
			tmpit->SetName(path);
			copied = (params->curtarget->itemhash->Get(*tmpit) != NULL);
		}
	} else
	{
		if (cur < 0)
			cur = -1;
		if (cur > items.GetN())
			cur = items.GetN();
		fname = "";
		ext = "";
		path = "";
		type = ITEM_TYPE_NONE;
		copied = FALSE;
		filetime = 0;
		filesize = 0;
		mask_index = 0;

		params->lastitem = NULL;
	}
	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_POSITION);

	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_PATH);
	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_FILENAME);
	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_EXTENSION);
	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_TYPE);
	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COPIED);
	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_FILETIME);
	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_MASKINDEX);
	mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_FILESIZE);
}

bool mask_compare(char *str, const SPString & msk)
{
	// empty mask - not allowed!
	if (msk[0] == '\0')
		return false;

	// process multiple masks
	char *comma = strchr(msk, ',');
	int mlen = msk.GetLength();
	if (comma != NULL)
	{
		// if one of other masks match
		if (mask_compare(str, SPString(comma + 1)))
			return true;
		mlen = comma - *msk;
	}
	
	int end = 0, omaski = 0;
	int len = strlen(str);
	int ml;
	for (int i = 0; i < len && omaski <= mlen; omaski++)
	{
		const char *m = *msk + omaski;
		for (int j = omaski; j < mlen && *m != '*'; j++)
			m++;
        if (*m == '*')
			ml = m - msk - omaski;
		else
			ml = mlen - omaski + 1;
		BOOL fl0 = false;
		for (; i <= end; i++)
		{
			const char *tmp1 = str + i;
			const char *tmp2 = *msk + omaski;
			BOOL fl = true;
			for (int j = 0; (j < ml); j++, tmp1++, tmp2++)
			{
				if (tolower(*tmp1) != tolower(*tmp2 == ',' ? 0 : *tmp2) && *tmp2 != '?')
				{
					fl = false;
					break;
				}
			}
			if (fl)
			{
				omaski += ml;
				i += ml;
				end = len;
				fl0 = true;
				break;
			}
		}
		if (!fl0)
			return false;
	}
	return true;
}

static void items_sub_sort(int idx1, int idx2)
{
	if (idx1 == idx2)
		return;
	if (params->sort == ITEMS_SORT_NORMAL)
	{
		params->curitem->items.Sort(ItemsListCompareFuncNormal, idx1, idx2);
	} 
	else if (params->sort == ITEMS_SORT_INVERSE)
	{
		params->curitem->items.Sort(ItemsListCompareFuncReverse, idx1, idx2);
	}
	else if (params->sort == ITEMS_SORT_RANDOM)
	{
		int n = idx2 - idx1 + 1;
		for (int k = idx1; k <= idx2; k++)
		{
			int j;
			// find random swap pos
			do
			{
				j = (rand() % n) + idx1;
			} while (j == k);
			
			// swap
			Item *tmp = params->curitem->items[j];
			params->curitem->items[j] = params->curitem->items[k];
			params->curitem->items[k] = tmp;
		}
	}
}

static void items_list_sort()
{
	params->lastitem = NULL;
	if (params->curitem != NULL && params->sort != ITEMS_SORT_NONE)
	{
		int k, idx1 = 0, idx2 = 0, maxidx = 0;
		// save current index
		bool need_new_idx = params->curitem->cur >= 0 && params->curitem->cur < params->curitem->items.GetN();
		if (need_new_idx)
		{
			params->curitem->items[params->curitem->cur]->oldidx = params->curitem->cur;
		}
		
		for (k = 0; k < ITEM_TYPE_MAX && params->filter[k] != ITEM_TYPE_NONE; k++)
		{
			//idx1 = params->curitem->add4types[k];
			idx2 = (k < ITEM_TYPE_MAX && params->filter[k] != ITEM_TYPE_NONE)
				? params->curitem->add4types[k]-1 : params->curitem->items.GetN() - 1;
			if (idx2 + 1 > maxidx)
				maxidx = idx2 + 1;
			if (idx2 < idx1)
				continue;
			items_sub_sort(idx1, idx2);
			idx1 = idx2 + 1;
		}
		if (maxidx < params->curitem->items.GetN())
			items_sub_sort(maxidx, params->curitem->items.GetN() - 1);

		if (need_new_idx)
		{
			int cur = params->curitem->cur;
			params->curitem->cur = 0;
			for (k = 0; k < params->curitem->items.GetN(); k++)
			{
				if (params->curitem->items[k]->oldidx == cur)
				{
					params->curitem->items[k]->oldidx = -1;
					params->curitem->cur = k;
					break;
				}
			}
			params->curitem->Update();
		}
	}
}

void explorer_get_folder_name(SPString &folder)
{
	// skip the 'up' folder
	if (folder == ".." || folder.GetLength() < 1)
		return;
	folder.Replace('\\', '/');
	if (folder.Find("/") != 0)
		folder = "/" + folder;
	if (folder.ReverseFind('/') != folder.GetLength() - 1)
		folder = folder + "/";
}

void script_explorer_reset()
{
	if (params != NULL)
	{
		params->lists.DeleteObjects();
		params->playlists.DeleteObjects();
		params->curtarget = NULL;
		params->curitem = NULL;
		params->lastitem = NULL;
		params->folder = "";

		mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COUNT);
		mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_FOLDER);
		mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_DRIVE_LETTER);
	}
}

void on_explorer_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_EXPLORER_CHARSET:
		var->Set(params->iso_lang);
		break;
    case SCRIPT_VAR_EXPLORER_FOLDER:
		{
			SPString f = params->folder;
			if (f.ReverseFind('/') == f.GetLength() - 1)
				f = f.Left(f.GetLength() - 1);
			var->Set(f);
		}
		break;
	case SCRIPT_VAR_EXPLORER_DRIVE_LETTER:
		{
			SPString f;
			if (params->folder.FindNoCase("/hdd/") == 0)
			{
				f = params->folder.Mid(5);
				if (f != "")
				{
					static const char led_drive_table[] = { 'A', 'b', 'C', 'd', 'E', 'F', 'G', 'H',
								'I', 'J', 'k', 'L', 'm', 'N', 'O', 'P', 'q', 'r', 'S', 't', 'U', 'v' };
					char ch = (char)toupper(f[0]);
					if (ch > 'A' && ch < sizeof(led_drive_table) + 'A')
						ch = led_drive_table[ch - 'A'];
					f = SPString(ch, 1);
					var->Set(f);
					break;
				}
			}
			var->Set(f);
		}
		break;
	case SCRIPT_VAR_EXPLORER_TARGET:
		{
			SPString f = params->target;
			if (f.ReverseFind('/') == f.GetLength() - 1)
				f = f.Left(f.GetLength() - 1);
			var->Set(f);
		}
		break;
    case SCRIPT_VAR_EXPLORER_FILTER:
		{
			const char *flt[] = { "", "file", "track", "folder", "up", "dvd" };
			SPString filter;
			for (int i = 0; i < ITEM_TYPE_MAX; i++)
			{
				if (params->filter[i] != ITEM_TYPE_NONE)
				{
					if (filter != "")
						filter += ",";
					filter += flt[params->filter[i]];
				}
			}
			
			var->Set(filter);
		}
		break;
    case SCRIPT_VAR_EXPLORER_MASK1:
    case SCRIPT_VAR_EXPLORER_MASK2:
   	case SCRIPT_VAR_EXPLORER_MASK3:
   	case SCRIPT_VAR_EXPLORER_MASK4:
   	case SCRIPT_VAR_EXPLORER_MASK5:
		var->Set(params->mask[var_id - SCRIPT_VAR_EXPLORER_MASK1]);
		break;
    case SCRIPT_VAR_EXPLORER_PATH:
		var->Set(params->curitem != NULL ? params->curitem->path : SPString());
		break;
    case SCRIPT_VAR_EXPLORER_FILENAME:
		var->Set(params->curitem != NULL ? params->curitem->fname : SPString());
		break;
    case SCRIPT_VAR_EXPLORER_EXTENSION:
		var->Set(params->curitem != NULL ? params->curitem->ext : SPString());
		break;
    case SCRIPT_VAR_EXPLORER_TYPE:
		{
			int typ = params->curitem != NULL ? params->curitem->type : -1;
			StringPair::Set(var, item_type_pairs, typ);
		}
		break;
    case SCRIPT_VAR_EXPLORER_FILESIZE:
		{
			if (params->curitem == NULL)
				var->Set(0);
			else
			{
				int fsize = (params->curitem->filesize < 0x7fffffff) ? 
					(int)params->curitem->filesize : -(int)(params->curitem->filesize/1024);
				var->Set(fsize);
			}
		}
		break;
	case SCRIPT_VAR_EXPLORER_FILETIME:
		{
			static SPString ft;
			ft = "";
			if (params->curitem->filetime != 0)
				ft.Strftime("%d.%m.%Y %H:%M", params->curitem->filetime);
			var->Set(ft);
		}
		break;
	case SCRIPT_VAR_EXPLORER_MASKINDEX:
		var->Set(params->curitem != NULL ? params->curitem->mask_index : 0);
		break;
    case SCRIPT_VAR_EXPLORER_COUNT:
		var->Set(params->curitem != NULL ? params->curitem->items.GetN() : 0);
		break;
    case SCRIPT_VAR_EXPLORER_SORT:
		StringPair::Set(var, items_sort_pairs, params->sort);
		break;
    case SCRIPT_VAR_EXPLORER_POSITION:
		var->Set(params->curitem != NULL ? params->curitem->cur : -1);
		break;
    case SCRIPT_VAR_EXPLORER_COMMAND:
	case SCRIPT_VAR_EXPLORER_FIND:
		var->Set("");
		break;
	case SCRIPT_VAR_EXPLORER_COPIED:
		var->Set(params->curitem != NULL ? params->curitem->copied : 0);
		break;
	}
}

void on_explorer_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *)
{
	if (var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_VAR_EXPLORER_CHARSET:
		params->iso_lang = var->GetString();
		params->iso_lang_changed = true;
		params->list_changed = true;
		break;
    case SCRIPT_VAR_EXPLORER_FOLDER:
		{
			params->folder = var->GetString();
			if (params->folder.FindNoCase("/cdrom/") == 0 || params->folder.FindNoCase("/hdd/") == 0 || 
				params->folder.FindNoCase("/") == 0)
				params->list_changed = true;
			explorer_get_folder_name(params->folder);
		}
		break;
	case SCRIPT_VAR_EXPLORER_TARGET:
		{
			params->target = var->GetString();
			explorer_get_folder_name(params->target);
			params->curtarget = NULL;
			for (int i = 0; i < params->playlists.GetN(); i++)
			{
				if (params->target.FindNoCase(params->playlists[i]->folder) == 0)
				{
					params->curtarget = params->playlists[i];
					break;
				}
			}
		}
		break;
    case SCRIPT_VAR_EXPLORER_FILTER:
		{
			SPString filter = var->GetString();
			int i;
			for (i = 0; i < ITEM_TYPE_MAX; i++)
			{
				if (filter.FindNoCase("up") == 0)
					params->filter[i] = ITEM_TYPE_UP;
				else if (filter.FindNoCase("folder") == 0)
					params->filter[i] = ITEM_TYPE_FOLDER;
				else if (filter.FindNoCase("file") == 0)
					params->filter[i] = ITEM_TYPE_FILE;
				else if (filter.FindNoCase("track") == 0)
					params->filter[i] = ITEM_TYPE_TRACK;
				else if (filter.FindNoCase("dvd") == 0)
					params->filter[i] = ITEM_TYPE_DVD;
				else
					break;
				int nxt = filter.Find(',');
				if (nxt < 0)
					filter = "";
				filter = filter.Mid(nxt + 1);
			}
			for (; i < ITEM_TYPE_MAX; i++)
				params->filter[i] = ITEM_TYPE_NONE;
			params->list_changed = true;
		}
		break;
    case SCRIPT_VAR_EXPLORER_MASK1:
    case SCRIPT_VAR_EXPLORER_MASK2:
    case SCRIPT_VAR_EXPLORER_MASK3:
    case SCRIPT_VAR_EXPLORER_MASK4:
    case SCRIPT_VAR_EXPLORER_MASK5:
	    {
			SPString m = var->GetString();
			
			// for easy mask concatenation
			m.TrimLeft();
			m.TrimRight();
			if (m.GetLength() > 0 && m[(int)m.GetLength() - 1] != ',')
				m += SPString(",");
			params->mask[var_id - SCRIPT_VAR_EXPLORER_MASK1] = m;
			
			params->allmask = SPString();
			for (int i = 0; i < num_file_masks; i++)
				params->allmask += params->mask[i];

			params->list_changed = true;
		}
		break;
    case SCRIPT_VAR_EXPLORER_SORT:
		params->sort = (ITEMS_SORT)StringPair::Get(var, items_sort_pairs, ITEMS_SORT_NONE);
		items_list_sort();
		params->list_changed = true;
		break;
    case SCRIPT_VAR_EXPLORER_POSITION:
		if (params->curitem != NULL && params->curitem->items.GetN() > 0)
		{
			params->curitem->cur = var->GetInteger();
			params->curitem->Update();
		}
		break;
    case SCRIPT_VAR_EXPLORER_COMMAND:
		{
			// our generated random indexes aren't valid any more...
			if (params->curitem != NULL)
			{
				if (params->list_changed)
				{
					SPSafeDeleteArray(params->curitem->random_idx);
					SPSafeDeleteArray(params->curitem->random_ridx);
					params->curitem->cur_random_pos = -1;
					params->list_changed = false;
				}
			}

			SPString cmd = var->GetString();
			if (cmd.CompareNoCase("update") == 0)
			{
				int i;
				params->curitem = NULL;
				
				msg("Explorer: Update!\n");

				if (params->folder == "..")
				{
					if (params->lists.GetN() > 1)
					{
						int idx = params->lists.GetN() - 1;
						SPSafeDelete(params->lists[idx]);
						params->lists.Remove(idx);
						
						// if we don't need update
						if (params->lists[idx - 1]->allmask.CompareNoCase(params->allmask) != 0)
						{
							params->folder = params->lists[idx - 1]->folder;
						} else
						{
							params->curitem = params->lists[idx - 1];
							mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COUNT);
							params->curitem->Update();
							params->folder = params->curitem->folder;
							mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_FOLDER);
							mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_DRIVE_LETTER);
							break;
						}
					} else
					{
						mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COUNT);
						params->folder = "";
						mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_FOLDER);
						mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_DRIVE_LETTER);
						break;
					}
				}

				// translate virtual path
				SPString oldfolder = params->folder;
				params->folder = cdrom_getrealpath(params->folder);
				if (params->folder != oldfolder)
					mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_FOLDER);
				mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_DRIVE_LETTER);
				
				if (params->folder.FindNoCase("/cdrom/") == 0 || params->folder.FindNoCase("/hdd/") == 0)
				{
					int from_which = -1;
					for (i = params->lists.GetN() - 1; i >= 0; i--)
					{
						if (params->folder.FindNoCase(params->lists[i]->folder) == 0)
						{
							// exactly the same!
							if (params->folder.CompareNoCase(params->lists[i]->folder) == 0)
							{
#if 0
								params->curitem = params->lists[i];
								mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COUNT);
								params->curitem->Update();
								params->folder = params->curitem->folder;
								mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_FOLDER);
								mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_DRIVE_LETTER);
								break;
#endif
								from_which = i - 1;
							}
							else
								from_which = i;
							break;
						}
					}
					// delete sub-lists
					for (i = params->lists.GetN() - 1; i > from_which; i--)
					{
						SPSafeDelete(params->lists[i]);
						params->lists.Remove(i);
					}
				}
				
				// detect source type
				if (params->folder.FindNoCase("/cdrom/") == 0 || params->folder.FindNoCase("/hdd/") == 0 ||
					params->folder == "/")
				{
					params->curitem = new ItemsList();

					// read items
					explorer_get_folder_name(params->folder);
					params->curitem->folder = params->folder;
					params->curitem->allmask = params->allmask;
					for (i = 0; i < num_file_masks; i++)
						params->curitem->mask[i] = params->mask[i];

					srand((unsigned int)time(NULL));

					// first, add audio tracks (for root folder only!)
					char tmp[10];
					if (params->folder.CompareNoCase("/cdrom/") == 0)
					{
						Cdda *cdda = cdda_open();
						if (cdda != NULL)
						{
							for (i = 0; i < cdda->tracks.GetN(); i++)
							{
								Item *it = new Item();
								it->parent = params->curitem;
								sprintf(tmp, "%02d.cda", i + 1);
								// we don't need hash for '/cdrom' lists
								if (params->curitem->itemhash == NULL)
								{
									it->name = tmp;
									it->namehash = 0;	
								} else
									it->SetName(tmp);
								it->datetime = 0;
								it->mask_index = 0;
								it->size = 0;
								it->type = ITEM_TYPE_TRACK;
								bool was = false, filterset = false;
								for (int ins = 0; ins < ITEM_TYPE_MAX; ins++)
								{
									if (params->filter[ins] != ITEM_TYPE_NONE)
										filterset = true;
									if (params->filter[ins] == ITEM_TYPE_TRACK)
									{
										params->curitem->items.Insert(it, params->curitem->add4types[ins]);
										was = true;
									}
									if (was)
										params->curitem->add4types[ins]++;
								}
								if (!filterset)
									params->curitem->items.Add(it);
								else if (!was)
									delete it;
							}
						} 
					}

					//if (has_data)
					{
						if (!cdrom_ismounted() || params->iso_lang_changed)
						{
							cdrom_mount(params->iso_lang, FALSE);
							params->iso_lang_changed = false;
						}

						// determine DVD folder
						bool is_dvd = false;
						int p = params->folder.FindNoCase("VIDEO_TS/");
						if (p >= 0 && p == params->folder.GetLength() - 9)
						{
							is_dvd = true;
						}

						bool root_folder = (params->folder == "/");

						DIR *dir = cdrom_opendir(params->folder);
						static char path[4097];
						strcpy(path, params->folder);
						int path_add = strlen(path);
						if (path_add < 4000)
						while (dir != NULL)
						{
							struct dirent *d = cdrom_readdir(dir);
							if (d == NULL)
								break;
							if (d->d_name[0] == '.' && d->d_name[1] == '\0')
								continue;
							struct stat64 statbuf;
							
							strcpy(path + path_add, d->d_name);
							int mask_index = 0;
							if (cdrom_stat(path, &statbuf) < 0) 
							{
								msg("Cannot stat %s\n", path);
								statbuf.st_mode = 0;
							}
							ITEM_TYPE ittype;
							if (!root_folder && d->d_name[0] == '.' && d->d_name[1] == '.' && d->d_name[2] == '\0')
							{
								if (params->lists.GetN() == 0)
									continue;
								ittype = ITEM_TYPE_UP;
							}
							else if (S_ISDIR(statbuf.st_mode))
							{
								if (d->d_name[0] == '.')	// hidden
									continue;
								
								// don't show other root folders except mounted ones.
								if (root_folder)
								{
									if (strcasecmp(d->d_name, "cdrom") != 0 &&
										strcasecmp(d->d_name, "hdd") != 0)
										continue;
								}
								ittype = ITEM_TYPE_FOLDER;
							}
							else
							{
								if (is_dvd && (strcasecmp(d->d_name, "VIDEO_TS.IFO") == 0 ||
										strcasecmp(d->d_name, "VIDEO_TS.BUP") == 0))
								{
									ittype = ITEM_TYPE_DVD;
									is_dvd = false;
								} else
								{
									ittype = ITEM_TYPE_FILE;
									BOOL found = FALSE;
									for (int mi = 0; mi < num_file_masks; mi++)
									{
										if (mask_compare(d->d_name, params->mask[mi]))
										{
											found = TRUE;
											mask_index = mi + 1;
											break;
										}
									}
									if (!found)
										continue;
								}
							}
							Item *it = new Item();
							it->parent = params->curitem;
							// we don't need hash for '/cdrom' lists
							if (params->curitem->itemhash == NULL)
							{
								it->name = d->d_name;
								it->namehash = 0;	
							} else
								it->SetName(d->d_name);
							it->datetime = statbuf.st_mtime;
							it->size = ittype == ITEM_TYPE_FILE ? statbuf.st_size : 0;
							it->type = ittype;
							it->mask_index = mask_index;
							bool was = false, filterset = false;
							for (int ins = 0; ins < ITEM_TYPE_MAX; ins++)
							{
								if (params->filter[ins] != ITEM_TYPE_NONE)
									filterset = true;
								if (params->filter[ins] == ittype)
								{
									params->curitem->items.Insert(it, params->curitem->add4types[ins]);
									was = true;
								}
								if (was)
								{
									params->curitem->add4types[ins]++;
								}
							}
							if (!filterset)
								params->curitem->items.Add(it);
							else if (!was)
								delete it;

	//					script_update();
						}
						cdrom_closedir(dir);
					}

					if (params->sort != ITEMS_SORT_NONE)
						items_list_sort();

					params->lists.Add(params->curitem);
				}
				else if (params->folder.FindNoCase("/dvd/") == 0)
				{
				}
				else // other playlists
				{
					for (i = 0; i < params->playlists.GetN(); i++)
					{
						if (params->folder.FindNoCase(params->playlists[i]->folder) == 0)
						{
							params->curitem = params->playlists[i];
							// If we need to apply a new mask to playlist,
							// we use actual list in itemhash to create filtered items list.
							if (params->curitem->allmask.CompareNoCase(params->allmask) != 0
								&& params->curitem->itemhash != NULL)
							{
								params->curitem->items.Clear();
								Item *cur = params->curitem->itemhash->GetFirst();
								while (cur != NULL)
								{
									if (mask_compare(cur->name, params->allmask))
										params->curitem->items.Add(cur);
									cur = params->curitem->itemhash->GetNext(*cur);
								}
								params->curitem->allmask = params->allmask;
								for (int mi = 0; mi < num_file_masks; mi++)
									params->curitem->mask[mi] = params->mask[mi];
								// we must apply sorting because hash list is unordered.
								ITEMS_SORT oldsort = params->sort;
								if (params->sort == ITEMS_SORT_NONE)
									params->sort = ITEMS_SORT_NORMAL;
								items_list_sort();
								params->sort = oldsort;
							}
							break;
						}
					}
				}
				
				mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COUNT);
				
				if (params->curitem != NULL)
				{
					Item *lastit = params->lastitem;
					params->curitem->cur = 0;
					params->curitem->Update();
					if (lastit != NULL)
						params->lastitem = lastit;
				}
			}
			else if (cmd.CompareNoCase("first") == 0)
			{
				if (params->curitem != NULL && params->curitem->items.GetN() > 0)
				{
					params->curitem->cur = 0;
					params->curitem->Update();
				}
			}
			else if (cmd.CompareNoCase("last") == 0)
			{
				if (params->curitem != NULL && params->curitem->items.GetN() > 0)
				{
					params->curitem->cur = params->curitem->items.GetN() - 1;
					params->curitem->Update();
				}
			}
			else if (cmd.CompareNoCase("next") == 0)
			{
				if (params->curitem != NULL && params->curitem->items.GetN() > 0 && params->curitem->cur < params->curitem->items.GetN())
				{
					params->curitem->cur++;
					params->curitem->Update();
				}
			}
			else if (cmd.CompareNoCase("prev") == 0)
			{
				if (params->curitem != NULL && params->curitem->items.GetN() > 0 && params->curitem->cur >= 0)
				{
					params->curitem->cur--;
					params->curitem->Update();
				}
			}
			else if (cmd.CompareNoCase("randomize") == 0)
			{
				if (params->curitem != NULL && params->curitem->items.GetN() > 0)
				{
					int k, n = params->curitem->items.GetN();
					SPSafeDeleteArray(params->curitem->random_idx);
					SPSafeDeleteArray(params->curitem->random_ridx);
					params->curitem->random_idx = new int [n];
					params->curitem->random_ridx = new int [n];
					for (k = 0; k < n; k++)
						params->curitem->random_idx[k] = k;
					if (n > 1)
					for (k = 0; k < n; k++)
					{
						int j; // find random swap pos
						do
						{
							j = (rand() % n);
						} while (j == k);
						// swap
						int tmp = params->curitem->random_idx[j];
						params->curitem->random_idx[j] = params->curitem->random_idx[k];
						params->curitem->random_idx[k] = tmp;
					}
					for (k = 0; k < n; k++)
						params->curitem->random_ridx[params->curitem->random_idx[k]] = k;
					params->curitem->cur_random_pos = 0;
					params->curitem->cur = params->curitem->random_idx[params->curitem->cur_random_pos];
					params->curitem->Update();
				}
			}
			else if (cmd.CompareNoCase("nextrandom") == 0 || cmd.CompareNoCase("prevrandom") == 0)
			{
				if (params->curitem != NULL && params->curitem->items.GetN() > 0)
				{
					if (params->curitem->random_idx != NULL && params->curitem->random_ridx != NULL)
					{
						if (params->curitem->cur >= 0 && params->curitem->cur < params->curitem->items.GetN())
							params->curitem->cur_random_pos = params->curitem->random_ridx[params->curitem->cur];

						if (cmd.CompareNoCase("nextrandom") == 0)
							params->curitem->cur_random_pos++;
						else
							params->curitem->cur_random_pos--;

						if (params->curitem->cur_random_pos < 0 || params->curitem->cur_random_pos >= params->curitem->items.GetN())
						{
							params->curitem->cur = -1;
							params->curitem->cur_random_pos = -1;
						} else
						{
							params->curitem->cur = params->curitem->random_idx[params->curitem->cur_random_pos];
						}
						params->curitem->Update();
					}
				}
				
			}
			else if (cmd.CompareNoCase("remove") == 0)
			{
				if (params->curitem != NULL && params->curitem->items.GetN() > 0 && 
					params->curitem->cur >= 0 && params->curitem->cur < params->curitem->items.GetN())
				{
					if (params->curitem->itemhash != NULL)
						params->curitem->itemhash->Remove(*params->curitem->items[params->curitem->cur]);
					SPSafeDelete(params->curitem->items[params->curitem->cur]);
					params->curitem->items.Remove(params->curitem->cur);
					if (params->curitem->cur == params->curitem->items.GetN() - 1)
						params->curitem->cur--;
					mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COUNT);
					params->curitem->Update();
				}
			}
			else if (cmd.CompareNoCase("removeall") == 0)
			{
				if (params->curitem != NULL)
				{
					if (params->curitem->itemhash != NULL)
						params->curitem->itemhash->Clear();
					params->curitem->items.DeleteObjects();
					params->curitem->cur = -1;
					mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COUNT);
					params->curitem->Update();
				}
			}
			else if (cmd.CompareNoCase("copy") == 0 || cmd.CompareNoCase("copyall") == 0)
			{
				// first, find existing play-list
				explorer_get_folder_name(params->target);
				
				int i;
				ItemsList *pl = NULL;
				for (i = params->playlists.GetN() - 1; i >= 0; i--)
				{
					// exactly the same!
					if (params->target.CompareNoCase(params->playlists[i]->folder) == 0)
					{
						pl = params->playlists[i];
						break;
					}
				}
				if (pl == NULL)	// create a new play-list
				{
					pl = new ItemsList();
					if (pl == NULL)
						break;
					pl->allmask = params->allmask;
					for (int mi = 0; mi < num_file_masks; mi++)
						pl->mask[mi] = params->mask[mi];
					pl->folder = params->target;

					pl->itemhash = new SPHashListAbstract<Item, Item>(items_num_hash);

					params->playlists.Add(pl);
				}
				params->curtarget = pl;
				if (params->curitem == NULL || pl == params->curitem)
				{
					msg_error("Cannot copy items from current source.\n");
					break;
				}
				bool copyall = cmd.CompareNoCase("copyall") == 0;
				
				if (!copyall)
				{
					if (params->curitem->cur < 0 || params->curitem->cur >= params->curitem->items.GetN())
					{
						msg_error("Cannot copy current item.\n");
						break;
					}
					i = params->curitem->cur;
				} else
					i = 0;
				for (; i < params->curitem->items.GetN(); i++)
				{
					Item *srcit = params->curitem->items[i];
					if (srcit != NULL && (srcit->type == ITEM_TYPE_FILE || srcit->type == ITEM_TYPE_TRACK))
					{
						// check if item already exists
						Item *item = NULL;
						static SPString nam;
						nam = params->folder + srcit->name;
						if (pl->itemhash != NULL)
						{
							tmpit->SetName(nam);
							item = pl->itemhash->Get(*tmpit);
						}
						// not in the playlist yet.
						if (item == NULL)
						{
							item = new Item();
							item->SetName(nam);
							item->type = srcit->type;
							item->datetime = srcit->datetime;
							item->mask_index = srcit->mask_index;
							item->size = srcit->size;
							item->parent = srcit->parent;	// inherit parent
							pl->itemhash->Add(item);
							
							srcit->playlist = pl;
							srcit->playlist_idx = pl->items.Add(item);
							if (i == params->curitem->cur)
								params->curitem->Update();
						}
					}
					if (!copyall)
						break;
				}
			}
			else if (cmd.CompareNoCase("targetremove") == 0 || cmd.CompareNoCase("targetremoveall") == 0)
			{
				// first, find existing play-list
				explorer_get_folder_name(params->target);
				
				ItemsList *pl = NULL;
				for (int i = params->playlists.GetN() - 1; i >= 0; i--)
				{
					// exactly the same!
					if (params->target.CompareNoCase(params->playlists[i]->folder) == 0)
					{
						pl = params->playlists[i];
						break;
					}
				}
				if (pl == NULL)	// create a new play-list
				{
					msg_error("Cannot find target playlist.\n");
					break;
				}
				params->curtarget = pl;
				if (params->curitem == NULL)
				{
					msg_error("Cannot use items from current source for removing.\n");
					break;
				}
				
				bool removed = false;
				if (cmd.CompareNoCase("targetremoveall") == 0)
				{
					pl->itemhash->Clear();
					pl->items.DeleteObjects();
					removed = true;
				} else
				{

					if (params->curitem->cur < 0 || params->curitem->cur >= params->curitem->items.GetN())
					{
						msg_error("Cannot remove current item from target playlist.\n");
						break;
					}
					Item *srcit = params->curitem->items[params->curitem->cur];
					if (srcit != NULL)
					{
						// check if item already exists
						Item *item = NULL;
						SPString nam = params->folder + srcit->name;
						if (pl->itemhash != NULL)
						{
							tmpit->SetName(nam);
							item = pl->itemhash->Get(*tmpit);
						}
						// not in the playlist yet.
						if (item == NULL)
						{
							msg_error("Cannot find current item in the target playlist.\n");
							break;
						}
						pl->itemhash->Remove(*item);
						// find item in the linear list
						for (int i = 0; i < pl->items.GetN(); i++)
						{
							if (pl->items[i] == item)
							{
								SPSafeDelete(pl->items[i]);
								pl->items.Remove(i);
								removed = true;
								break;
							}
						}
					}
				}
				
				if (removed)
				{
					if (params->curitem == pl)
					{
						if (params->curitem->cur == params->curitem->items.GetN() - 1)
							params->curitem->cur--;
						mmsl->UpdateVariable(SCRIPT_VAR_EXPLORER_COUNT);
					}
					params->curitem->Update();
				}
			}
		}
		break;
	case SCRIPT_VAR_EXPLORER_FIND:
		{
			SPString fpath = var->GetString();
			if (params->curitem != NULL && params->curitem->items.GetN() > 0)
			{
				// first, a little optimization for current items in playlist
				if (params->lastitem != NULL && params->lastitem->parent != NULL
					&& fpath.CompareNoCase(params->lastitem->parent->folder + params->lastitem->name) == 0
					&& params->lastitem->playlist == params->curitem
					&& params->lastitem->playlist_idx >= 0 
					&& params->lastitem->playlist_idx < params->curitem->items.GetN())
				{
					params->curitem->cur = params->lastitem->playlist_idx;
				}
				else
				{
					params->curitem->cur = -1;
					for (int i = 0; i < params->curitem->items.GetN(); i++)
					{
						Item *ci = params->curitem->items[i];
						if (fpath.CompareNoCase(params->curitem->folder + ci->name) == 0)
						{
							params->curitem->cur = i;
							break;
						}
					}
				}
				params->curitem->Update();
			}
		}
		break;
	}

}

