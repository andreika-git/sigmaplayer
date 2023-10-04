//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - module system impl.
 *  \file       module.cpp
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

#include <module.h>

extern "C" {

typedef int (*MODULE_INIT_PROC)(MODULE_DESC *desc, MODULE_FUNC_TABLE *client, MODULE_FUNC_TABLE *server);

MODULE_INIT_PROC module_init;

MODULE_INIT_PROC module_getproc(char *str)
{
	if (str == 0)
		return 0;

	// str - decimal pointer
	unsigned long proc = 0;
	for (char *s = str; *s != '\0'; s++)
	{
		if (*s < '0' || *s > '9')
			return 0;
		proc *= 10;
		proc += (*s - '0');
	}

	return (MODULE_INIT_PROC)proc;
}

void module_start(char *interface_ptr_str, MODULE_DESC *desc, MODULE_FUNC_TABLE *client, MODULE_FUNC_TABLE *server)
{
	module_init = module_getproc(interface_ptr_str);
	if (module_init != 0)
	{
		if (module_init(desc, client, server) < 0)
			return;
	}
}

// end of extern "C"
};

#else	// server side

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_module.h>

#include <module.h>
#include <settings.h>

extern "C" {

const int MAX_NUM_MODULES = 32;
typedef struct MODULE_STRUCT
{
	volatile BOOL loaded;
	SPString name;
	DWORD hash;
	MODULE_DESC *desc;
	int pid;
	MODULE_FUNC_TABLE *client_table;
} MODULE_STRUCT;

MODULE_STRUCT modules[MAX_NUM_MODULES];
int num_modules = 0;
static volatile bool was_module_error = false;

/////////////////////////////
extern MODULE_FUNC_TABLE dvd_funcs[];
extern MODULE_FUNC_TABLE init_funcs[];
/////////////////////////////

int module_find(const SPString & name)
{
	// find module
	DWORD hash = name.Hash();
	int mod_idx = -1;
	for (int i = 0; i < num_modules; i++)
	{
		if (modules[i].hash == hash)
		{
			if (modules[i].name.CompareNoCase(name) == 0)
			{
				mod_idx = i;
				break;
			}
		}
	}
	return mod_idx;
}

int module_copy_table(MODULE_FUNC_TABLE *from, MODULE_FUNC_TABLE *to)
{
	for (int i = 0; to[i].addr != NULL; i++)
	{
		int id = to[i].id;
		bool found = false;
		for (int j = 0; from[j].addr != NULL; j++)
		{
			if (from[j].id == id)
			{
				found = true;
				module_copy_func(from[j].addr, to[i].addr);
				break;
			}
		}
		if (!found)
		{
			msg("Module: Cannot find function entry, ID=%d!\n", id);
			return -1;
		}
	}
	return 0;
}

/// Called from module!
int module_init(MODULE_DESC *desc, MODULE_FUNC_TABLE *client, MODULE_FUNC_TABLE *server)
{
	was_module_error = false;
	if (desc == NULL || client == NULL || server == NULL)
	{
		was_module_error = true;
		return -1;
	}

	msg("Module: Starting '%s'...\n", desc->name);
	if (desc->init_ver > SP_VERSION)
	{
		msg("Module: Required version mismatch (%d.%d > %d.%d)!\n", desc->init_ver / 100, desc->init_ver % 100,
			SP_VERSION / 100, SP_VERSION % 100);
		was_module_error = true;
		return -1;
	}
	int mod_idx = module_find(desc->name);
	if (mod_idx < 0)
	{
		msg("Module: Unknown module loaded - '%s'!\n", desc->name);
		was_module_error = true;
		return -1;
	}
	modules[mod_idx].desc = desc;

	// copy dvd tables
	MODULE_FUNC_TABLE *client_table = NULL;
	
	if (strcasecmp(desc->class_name, "dvd") == 0)
		client_table = dvd_funcs;
	if (client_table == NULL)
	{
		msg("Module: Cannot find client table for module '%s'.\n", desc->name);
		was_module_error = true;
		return -1;
	}
	if (module_copy_table(client, client_table) < 0)
	{
		was_module_error = true;
		return -1;
	}
	if (module_copy_table(init_funcs, server) < 0)
	{
		was_module_error = true;
		return -1;
	}
	
	modules[mod_idx].client_table = client_table;
	modules[mod_idx].loaded = TRUE;

	msg("Module: '%s' Started.\n", desc->name);

	return 0;
}
	

int module_load(const SPString & name)
{
	int mod_idx = module_find(name);
	if (mod_idx < 0)	// add new module
	{
		if (num_modules >= MAX_NUM_MODULES)
		{
			msg("Module: Max. modules number limit exceeded!\n");
			return -1;
		}
		mod_idx = num_modules++;
		modules[mod_idx].name = name;
		modules[mod_idx].hash = name.Hash();
		modules[mod_idx].loaded = FALSE;
		modules[mod_idx].desc = NULL;
		modules[mod_idx].client_table = NULL;
	} else
	{
		if (modules[mod_idx].loaded)	// it's already loaded!
			return 0;
	}

	char fname[128], arg[32];
	sprintf(fname, "/modules/%s.bin", (char *)name);
	sprintf(arg, "%lu", (DWORD)module_init);

	msg("Module: Loading '%s'...\n", (char *)name);

	was_module_error = false;
	int pid = module_binary_load(fname, arg);
	if (pid <= 0)
	{
		msg("Module: Cannot load module '%s'!\n", (char *)name);
		return -1;
	}

	modules[mod_idx].pid = pid;
	
	// now we'll wait for process to settle:
	for (int tries = 0; tries < 1000; tries++)
	{
		if (modules[mod_idx].loaded)
			break;
		if (was_module_error)
		{
			msg("Module: Error was detected during module_init()!\n");
			break;
		}
		usleep(100);
	}
	if (!modules[mod_idx].loaded)
	{
		msg("Module: Cannot init module '%s'.\n", (char *)name);
		module_unload(name);
		return -1;
	}
	
	msg("Module: '%s' Loaded!\n", (char *)name);
	return 0;
}

int module_unload(const SPString & name)
{
	int mod_idx = module_find(name);
	if (mod_idx < 0)
		return -1;
	if (!modules[mod_idx].loaded)
	{
		msg("Module: Unload - Module '%s' is not loaded!\n", (char *)name);
		return -1;
	}
	if (module_binary_unload(modules[mod_idx].pid) >= 0)
	{
		if (modules[mod_idx].client_table != NULL)
		{
			MODULE_FUNC_TABLE *tbl = modules[mod_idx].client_table;
			for (int i = 0; tbl[i].addr != NULL; i++)
			{
				module_clear_func(modules[mod_idx].client_table);
			}
		}
		modules[mod_idx].pid = -1;
		modules[mod_idx].loaded = FALSE;
	}
	return 0;
}

// end of extern "C"
};

#endif
