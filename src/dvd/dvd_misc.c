//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - libdvdnav cache replacement
 *  \file       dvd/dvd_misc.c
 *  \author     bombur
 *  \version    0.1
 *  \date       1.02.2006
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <inttypes.h>

#include "dvdnav.h"
#include "dvdnav_internal.h"
#include "read_cache.h"

#include "dvd_misc.h"

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_khwl.h>
#include <libsp/sp_mpeg.h>
#include "media.h"


///////////// DVD Caching...
/*
	[bombur]: let me describe the situation.
	- dvdnav_read_cache_block() is called several times
	- we have 8 buffers; each contains 16 blocks max.
	- we won't precache at the vobu start
	- we won't allocate any buffers or memory here
*/

///////////////////////
//#define NO_CACHE
///////////////////////

// must be equal to MPEG_NUM_BUFS
#define READ_CACHE_CHUNKS 8

// must be equal to MPEG_BUF_SIZE
#define READ_CACHE_BUF_SIZE 32768

#define READ_AHEAD_SIZE (READ_CACHE_BUF_SIZE / DVD_VIDEO_LB_LEN)

typedef struct read_cache_chunk_s 
{
  uint8_t      *cache_buffer;
  int32_t		cache_start_sector; /* -1 means cache invalid */
  int32_t		cache_read_count;   /* this many sectors are already read */
  size_t		cache_block_count;  /* this many sectors will go in this chunk */
  size_t		cache_malloc_size;
  int			cache_missed;		// used when a cache-missed block was written
} read_cache_chunk_t;

struct read_cache_s 
{
  read_cache_chunk_t  chunk[16 /*READ_CACHE_CHUNKS */];
  int                 current;
  int                 last_sector;
  int                 wait_for_new_buffer;
  int                 called_flag;

  int                 range_start;	// save VOBU sector range to avoid unused cache
  int                 range_count;

  /* Bit of strange cross-linking going on here :) -- Gotta love C :) */
  dvdnav_t           *dvd_self;
};

read_cache_t *dvdnav_read_cache_new(dvdnav_t* dvd_self) 
{
	read_cache_t *self;
	int i;

	self = (read_cache_t *)malloc(sizeof(read_cache_t));

	if(self) 
	{
		self->current = 0;
		self->dvd_self = dvd_self;
		
		self->wait_for_new_buffer = 1;
		self->called_flag = 0;
		self->last_sector = 0;
		self->range_start = 0;
		self->range_count = 0;

		dvdnav_read_cache_clear(self);
		for (i = 0; i < READ_CACHE_CHUNKS; i++) 
		{
			self->chunk[i].cache_buffer = NULL;
			self->chunk[i].cache_read_count = 0;
			self->chunk[i].cache_block_count = READ_AHEAD_SIZE;
		}
	}

	return self;
}

void dvdnav_read_cache_free(read_cache_t* self) 
{
	dvdnav_t *tmp;

	/* all buffers returned, free everything */
	tmp = self->dvd_self;
	free(self);
	free(tmp);
}

/* This function MUST be called whenever self->file changes. */
void dvdnav_read_cache_clear(read_cache_t *self) 
{
   return;
}

/* This function is called just after reading the NAV packet. */
void dvdnav_pre_cache_blocks(read_cache_t *self, int sector, size_t block_count) 
{
    self->range_start = sector;
	self->range_count = block_count;
	return;
}

// This is our function - pass 'khwl' buffer pointers
int dvdnav_set_cache_memory(dvdnav_t* dvd_self, uint8_t *block_mem)
{
	read_cache_t *self;
	int i;

	self = dvd_self->cache;

	if (self) 
	{
		for (i = 0; i < READ_CACHE_CHUNKS; i++)
		{
			self->chunk[i].cache_buffer = block_mem;
			self->chunk[i].cache_malloc_size = READ_CACHE_BUF_SIZE;
			block_mem += READ_CACHE_BUF_SIZE;
		}
	}
	return DVDNAV_STATUS_OK;
}

void dvdnav_reset_cache_flag(dvdnav_t* dvd_self)
{
	if (dvd_self->cache)
		dvd_self->cache->called_flag = 0;
}
int dvdnav_get_cache_flag(dvdnav_t* dvd_self)
{
	return (dvd_self->cache != NULL) ? dvd_self->cache->called_flag : 0;
}

int dvdnav_read_cache_block(read_cache_t *self, int sector, size_t block_count, uint8_t **buf) 
{
	int i, use, count;
	int32_t res;

	if(!self)
		return 0;

	self->called_flag = 1;

retry:
	use = -1;
#ifndef NO_CACHE
	// sorry, our read-ahead cache works with only 1 block at a time
	if(self->dvd_self->use_read_ahead && block_count == 1) 
	{
		// find our chunk - try the current chunk first
		read_cache_chunk_t cur = self->chunk[self->current];
		if (*buf >= cur.cache_buffer && *buf < (cur.cache_buffer + cur.cache_malloc_size))
			use = self->current;
		else
		{
			// search others
			for (i = 0; i < READ_CACHE_CHUNKS; i++)
			{
				if (*buf >= self->chunk[i].cache_buffer &&
						*buf < (self->chunk[i].cache_buffer + self->chunk[i].cache_malloc_size))
					use = i;
			}
		}
	}
#endif
	if (use >= 0)
	{
		read_cache_chunk_t *chunk;
		chunk = &self->chunk[use];
		self->current = use;
		self->last_sector = sector;
		// this is not the first buffer, so try searching the cache
		if (*buf != chunk->cache_buffer)
		{
			// we want to return already filled buf - so we check the sector bounds
			if (!chunk->cache_missed && sector >= chunk->cache_start_sector &&
					sector < (chunk->cache_start_sector + chunk->cache_read_count) &&
					sector + block_count <= chunk->cache_start_sector + chunk->cache_block_count)
			{
				if (*buf == chunk->cache_buffer + (sector - chunk->cache_start_sector) * DVD_VIDEO_LB_LEN)
				{
					return DVD_VIDEO_LB_LEN;
				}
			}
			// ask another free buf
			msg("--cache skip\n");
			if (media_skip_buffer(buf))
				goto retry;
			// if not, then cache-miss
		}
		else
		{
			// start a new chunk - read it entirely
			chunk->cache_missed = 0;
			chunk->cache_start_sector = sector;

			count = chunk->cache_block_count;

#if 0
			// if we don't want to get out of VOBU range...
			if (self->range_count > 0)
			{
				if (sector >= self->range_start 
						&& sector < self->range_start + self->range_count
						&& sector + count >= self->range_start + self->range_count)
				{
					count = self->range_start + self->range_count - sector;
					msg("--cache limited to %d\n", count);
				}
			}
#endif

			chunk->cache_read_count = (int32_t)DVDReadBlocks(self->dvd_self->file,
													chunk->cache_start_sector,
													count,
													chunk->cache_buffer);
			if (chunk->cache_read_count > 0)
			{
				self->wait_for_new_buffer = 0;
				if (*buf == chunk->cache_buffer)
				{
					return DVD_VIDEO_LB_LEN;
				}
			}
			else
				return 0;
		}
		chunk->cache_missed = 1;
	}

	if (self->dvd_self->use_read_ahead)
	{
		// cache miss...
		msg("--cache miss\n");
	}
	res = (int32_t)DVDReadBlocks(self->dvd_self->file,
							sector, 1, *buf) * DVD_VIDEO_LB_LEN;

	return res;

}

dvdnav_status_t dvdnav_free_cache_block(dvdnav_t *self, unsigned char *buf) 
{
	read_cache_t *cache;

	if (!self)
		return DVDNAV_STATUS_ERR;

	cache = self->cache;
	if (!cache)
		return DVDNAV_STATUS_ERR;

	return DVDNAV_STATUS_OK;
}

int8_t dvd_get_audio_logical_stream(dvdnav_t *dvd, uint8_t audio_num) 
{
	int audioN;
	if(!dvd) 
	{
		//printerr("Passed a NULL pointer.");
		return -1;
	}
	if(!dvd->started) 
	{
		//printerr("Virtual DVD machine not started.");
		return -1;
	}
  
	pthread_mutex_lock(&dvd->vm_lock);
	if (!dvd->vm->state.pgc) 
	{
		//printerr("No current PGC.");
		pthread_mutex_unlock(&dvd->vm_lock); 
		return -1;
	}
	
	for (audioN = 0; audioN < 8; audioN++)
	{
		if (vm_get_audio_stream(dvd->vm, audioN) == audio_num)
		{
			pthread_mutex_unlock(&dvd->vm_lock); 
			return audioN;
		}
	}

	pthread_mutex_unlock(&dvd->vm_lock); 
	return -1;
}


/// This is almost exact copy of 'libdvdnav' function, except aspect mode
int8_t dvd_get_spu_logical_stream(dvdnav_t *dvd, uint8_t subp_num, int mode) 
{
	int subpN;

	if(!dvd) 
	{
		//printerr("Passed a NULL pointer.");
		return -1;
	}
	if(!dvd->started) 
	{
		//printerr("Virtual DVD machine not started.");
		return -1;
	}

	if (!dvd->vm->state.pgc) 
	{
		//printerr("No current PGC.");
		return -1;
	}
	for (subpN = 0; subpN < 32; subpN++)
	{
		if (vm_get_subp_stream(dvd->vm, subpN, mode) == subp_num)
			return subpN;
	}
	return -1;
}

int dvd_set_spu_stream(dvdnav_t *dvd, int stream, int mode)
{
	int source_aspect = vm_get_video_aspect(dvd->vm);
	int req_s, subpN, streamN = -1;

	if((dvd->vm->state).domain != VTS_DOMAIN)
		return 0;

	for (req_s = stream; req_s >= 0 && req_s < 32; req_s++)
	{
		for (subpN = 0; subpN < 32; subpN++)
		{
			/* Is this logical stream present */ 
			if ((dvd->vm->state).pgc->subp_control[subpN] & (1<<31)) 
			{
				if(source_aspect == 0) /* 4:3 */	     
					streamN = ((dvd->vm->state).pgc->subp_control[subpN] >> 24) & 0x1f;  
				if (source_aspect == 3) /* 16:9 */
				{
					switch (mode) 
					{
					case 0:
						streamN = ((dvd->vm->state).pgc->subp_control[subpN] >> 16) & 0x1f;
						break;
					case 1:
						streamN = ((dvd->vm->state).pgc->subp_control[subpN] >> 8) & 0x1f;
						break;
					case 2:
						streamN = (dvd->vm->state).pgc->subp_control[subpN] & 0x1f;
					}
				}
				// that's it!
				if (streamN == req_s)
				{
					(dvd->vm->state).SPST_REG &= ~0x7f; // Keep other bits.
					(dvd->vm->state).SPST_REG |= 0x40; // Turn it on
					(dvd->vm->state).SPST_REG |= subpN;
					return 1;
				}
			}
		}
	}
	// not found, Turn it off
	(dvd->vm->state).SPST_REG &= ~0x40;
	return 1;
}

int dvd_set_audio_stream(dvdnav_t *dvd, int stream)
{
	int req_s, audioN;

	if((dvd->vm->state).domain != VTS_DOMAIN)
		return 0;

	for (req_s = stream; req_s >= 0 && req_s < 8; req_s++)
	{
		for (audioN = 0; audioN < 8; audioN++)
		{
			if((dvd->vm->state).pgc->audio_control[audioN] & (1<<15)) 
			{
				int streamN = ((dvd->vm->state).pgc->audio_control[audioN] >> 8) & 0x07;  
				if (streamN == req_s)
				{
					(dvd->vm->state).AST_REG = audioN;
					return 1;
				}
			}
		}
	}
	// not found, 
	(dvd->vm->state).AST_REG = 15;	// non-existant?
	return 1;
}


int dvd_get_time(dvdnav_t *dvd, int title, int chapter, uint64_t *time) 
{
	uint64_t length = 0;
	uint32_t first_cell_nr, last_cell_nr, cell_nr;
	int cur_title, cur_part;
	cell_playback_t *cell;
	dvd_state_t *state;
	int ttn;

	*time = 0;

	pthread_mutex_lock(&dvd->vm_lock);

	if (!dvd->vm->vmgi) 
	{
		//printerr("Bad VM state.");
		pthread_mutex_unlock(&dvd->vm_lock);
		return DVDNAV_STATUS_ERR;
	}
	if (!dvd->started) 
	{
	    /* don't report an error but be nice */
	    vm_start(dvd->vm);
	    dvd->started = 1;
	}

	if(dvd->position_current.still != 0) 
	{
		//printerr("Cannot seek in a still frame.");
		pthread_mutex_unlock(&dvd->vm_lock);
		return DVDNAV_STATUS_ERR;
	}
  
	state = &(dvd->vm->state);
	if(!state->pgc) 
	{
		//printerr("No current PGC.");
		pthread_mutex_unlock(&dvd->vm_lock);
		return DVDNAV_STATUS_ERR;
	}

	dvdnav_current_title_info(dvd, &cur_title, &cur_part);
	if (title >= 0 && title + 1 != cur_title)
	{
		//printerr("Cannot seek in a still frame.");
		pthread_mutex_unlock(&dvd->vm_lock);
		return DVDNAV_STATUS_ERR;
	}

	first_cell_nr = 1;
	if (title < 0 || chapter < 0)
		last_cell_nr = state->pgc->nr_of_cells;
	else 
	{
		vts_ptt_srpt_t *vts_ptt_srpt = dvd->vm->vtsi->vts_ptt_srpt;
		if (chapter == 0)
			last_cell_nr = 0;
		else
		{
			last_cell_nr = state->pgc->nr_of_cells;
			chapter--;
			ttn = dvd->vm->vmgi->tt_srpt->title[title].vts_ttn - 1;
			if (ttn >= 0 && ttn < vts_ptt_srpt->nr_of_srpts)
			{
				if (chapter < vts_ptt_srpt->title[ttn].nr_of_ptts)
				{
					int pgN = vts_ptt_srpt->title[ttn].ptt[chapter].pgn; // state->pgN
					if(pgN < state->pgc->nr_of_programs)
						last_cell_nr = state->pgc->program_map[pgN] - 1;
					else
						last_cell_nr = state->pgc->nr_of_cells;
				}
			}
		}
	}

	length = 0;
	for(cell_nr = first_cell_nr; (cell_nr <= last_cell_nr); cell_nr ++) 
	{
		cell =  &(state->pgc->cell_playback[cell_nr-1]);
		length += dvdnav_convert_time(&cell->playback_time);
	}

	*time = length;

	pthread_mutex_unlock(&dvd->vm_lock);
	return 1;
}

int dvd_get_chapter(dvdnav_t *dvd, int title, uint64_t time, int *chapter) 
{
	uint64_t length = 0;
	uint32_t last_cell_nr, cell_nr;
	int32_t found = 0;
	int chap = 0;
	int cur_title, cur_part;
	cell_playback_t *cell;
	dvd_state_t *state;
	vts_ptt_srpt_t *vts_ptt_srpt = dvd->vm->vtsi->vts_ptt_srpt;
	int ttn;

	*chapter = 0;

	pthread_mutex_lock(&dvd->vm_lock);

	if (!dvd->vm->vmgi) 
	{
		//printerr("Bad VM state.");
		pthread_mutex_unlock(&dvd->vm_lock);
		return DVDNAV_STATUS_ERR;
	}
	if (!dvd->started) 
	{
	    /* don't report an error but be nice */
	    vm_start(dvd->vm);
	    dvd->started = 1;
	}

	if (dvd->position_current.still != 0) 
	{
		//printerr("Cannot seek in a still frame.");
		pthread_mutex_unlock(&dvd->vm_lock);
		return DVDNAV_STATUS_ERR;
	}

	dvdnav_current_title_info(dvd, &cur_title, &cur_part);
	if (title + 1 != cur_title)
	{
		//printerr("Cannot seek in a still frame.");
		pthread_mutex_unlock(&dvd->vm_lock);
		return DVDNAV_STATUS_ERR;
	}
  	
	state = &(dvd->vm->state);
	if (!state->pgc || title < 0) 
	{
		//printerr("No current PGC.");
		pthread_mutex_unlock(&dvd->vm_lock);
		return DVDNAV_STATUS_ERR;
	}

	
	last_cell_nr = state->pgc->nr_of_cells;
	cell_nr = 1;
	length = 0;
	ttn = dvd->vm->vmgi->tt_srpt->title[title].vts_ttn - 1;
	if (ttn >= 0 && ttn < vts_ptt_srpt->nr_of_srpts)
	{
		for (chap = 0; chap < vts_ptt_srpt->title[ttn].nr_of_ptts; chap++)
		{
			int pgN = vts_ptt_srpt->title[ttn].ptt[chap].pgn; // state->pgN
			if(pgN < state->pgc->nr_of_programs)
				last_cell_nr = state->pgc->program_map[pgN] - 1;
			else
				last_cell_nr = state->pgc->nr_of_cells;
			for(; (cell_nr <= last_cell_nr) && !found; cell_nr ++) 
			{
				cell = &(state->pgc->cell_playback[cell_nr-1]);
				length += dvdnav_convert_time(&cell->playback_time);
				if (length > time)
				{
					found = 1;
					break;
				}
			}
			if (found)
				break;
		}		
	}

	*chapter = chap;
	
	pthread_mutex_unlock(&dvd->vm_lock);
	return 1;
}

// declared in vm.c
extern uint8_t dvd_name_data[DVD_VIDEO_LB_LEN];

static uint32_t crc_table[256];

void dvd_make_crc_table()
{
	uint32_t c, poly;            // polynomial exclusive-or pattern
	uint32_t n, k;
	// make exclusive-or pattern from polynomial (0xedb88320L)
	static const uint8_t p[] = { 0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26 };
	poly = 0L;
	for (n = 0; n < sizeof(p)/sizeof(uint8_t); n++)
		poly |= 1L << (31 - p[n]);

	for (n = 0; n < 256; n++)
	{
		c = n;
		for (k = 0; k < 8; k++)
		  c = (c & 1) ? poly ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}
}

uint32_t dvd_get_disc_ID(const char *use_name)
{
	int i, l;
	uint32_t crc32 = 0xffffffffL;
	uint8_t *data;

	if (use_name != NULL)
	{
		data = (uint8_t *)use_name;
		l = strlen(use_name);
	} else
	{
		data = dvd_name_data;
		l = 128;
	}
	
	for(i = 0; i < l; i++)
	{
		register uint8_t c = data[i];
		crc32 = crc_table[(crc32 & 0xff) ^ c] ^ (crc32 >> 8);
	}
	crc32 ^= 0xffffffffL;

#if 0
	{
	char d[130];
	for (i = 0; i < l; i++)
	{
		if (data[i] >= 32 && data[i] < 127)
			d[i] = data[i];
		else
			d[i] = ' ';
	}
	d[l] = '\0';
	msg("* [%s]\n", d);
	}
#endif
	msg("* DVD: ID = %8x\n", crc32);
	return crc32;
}

void dvd_reset_vm(dvdnav_t *dvd)
{
	vm_reset(dvd->vm, NULL);
	dvd->position_current.still = 0;
	dvd->sync_wait = 0;
}

int dvd_skip_sector(dvdnav_t *dvd)
{
	if (dvd->nav_packet_error)
	{
		dvd->vobu.vobu_start ++;
		return 0;
	}
	return -1;
}
