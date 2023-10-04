//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - module system header file
 *  \file       module.h
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

#ifndef SP_MODULE_H
#define SP_MODULE_H

typedef struct MODULE_FUNC_TABLE
{
	int id;
	void *addr;
} MODULE_FUNC_TABLE;

#define MODULE_DECL_START() enum {
#define MODULE_DECL_END() };

#ifdef MODULE_SERVER
#define MODULE_DECL(f)	modid_##f
#else
#define MODULE_DECL(f) modid_##f }; MODULE_FUNC(f); enum { modid_##f##_continue = modid_##f

#endif

#define MODULE_ENTRY(f) { modid_##f, ((void *)&f) }
#define MODULE_ENTRY_END() { -1, 0 }

typedef struct MODULE_DESC
{
	const char *name;			/// module name
	const char *class_name;	/// module class name
	const char *desc;			/// module description
	int ver;			/// module version (100 = 1.00)
	int init_ver;		/// minimal init (module sys.) version (100 = 1.00)
	
	int reserved[16];

} MODULE_DESC;

#ifdef COMPILE_MODULE
extern "C" 
{
void module_start(char *interface_ptr_str, MODULE_DESC *desc, MODULE_FUNC_TABLE *client, MODULE_FUNC_TABLE *server);
}
#else

#include <libsp/sp_misc.h>
extern "C" 
{
int module_load(const SPString & name);
int module_unload(const SPString & name);
}
#endif



#endif // of SP_MODULE_H
