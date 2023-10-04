//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - modules support functions header file
 *  \file       sp_module.h
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

#ifndef SP_SYSMODULE_H
#define SP_SYSMODULE_H

#ifdef WIN32
#define MODULE_FUNC(f) unsigned __int64 f
#else
#define MODULE_FUNC(f) unsigned long f __attribute__ ((__section__ (".text")))
//#define MODULE_FUNC(f) unsigned long f
#endif



#ifdef __cplusplus
extern "C" {
#endif

/// Load module binary (process ID is returned).
int module_binary_load(char *fname, char *arg);

/// Unload module binary from memory.
int module_binary_unload(int pid);

/// Freeze module process
void module_wait();

/// Write 'from' address to the 'to' memory location
void module_copy_func(void *from, void *to);

/// Set function location to NULL
void module_clear_func(void *func);

#ifdef __cplusplus
}
#endif

#endif // of SP_SYSMODULE_H
