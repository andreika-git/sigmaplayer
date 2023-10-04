//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - DVD player module main source file.
 *  \file       dvd/main.cpp
 *  \author     bombur
 *  \version    0.21
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

#include <unistd.h>
#include <libsp/sp_misc.h>
#include <libsp/sp_module.h>

#include <module.h>

extern "C"
{
	extern MODULE_FUNC_TABLE dvd_funcs[];
	extern MODULE_FUNC_TABLE init_funcs[];
	extern MODULE_DESC module_dvd_desc;
};

int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		module_start(argv[1], &module_dvd_desc, dvd_funcs, init_funcs);
		module_wait();
	}

	return 0;
}
