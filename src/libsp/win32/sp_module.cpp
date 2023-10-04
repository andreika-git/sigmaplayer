//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - Module system support source file.
 * 								For Win32.
 *  \file       sp_module.cpp
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

#include <stdio.h>
#include <windows.h>

extern "C"
{

// portions of code from uClibc...
#ifdef COMPILE_MODULE

extern int  main(int argc, char **argv, char **envp);

__declspec( dllexport ) void ModuleLink(char *fname, char *arg)
{
	char *argv[2];
	argv[0] = fname;
	argv[1] = arg;
	main(2, argv, NULL);
}

BOOL WINAPI DllMain(HINSTANCE , DWORD , LPVOID)
{
	return TRUE;
}

void module_wait()
{
}

#else	/////////////////////////////////////////////////////////////////////////

extern "C"
{
	typedef void (*ModuleLink_decl)(char *, char *);
	ModuleLink_decl ModuleLink;
}

int module_binary_load(char *fname, char *arg)
{
	char *fpath = (fname[0] == '/') ? fname + 1 : fname;
	HMODULE h = LoadLibrary(fpath);
	if (h == NULL)
	{
		DWORD err = GetLastError();
		printf("Module: Load error = %d.\n", err);
		return -1;
	}

	ModuleLink = (ModuleLink_decl)GetProcAddress(h, "ModuleLink");
	if (ModuleLink == NULL)
		return -1;
	ModuleLink(fname, arg);

	return (int)h;
}

int module_binary_unload(int pid)
{
	HMODULE h = (HMODULE)pid;
	return FreeLibrary(h) ? 0 : -1;
}

void module_copy_func(void *from, void *to)
{
	#define JMP_OPCODE_SIZE 5
	ULONGLONG diff = (DWORD)from - (DWORD)to - JMP_OPCODE_SIZE;
	*((ULONGLONG *)to) = 0xE9 | (diff << 8);
}

void module_clear_func(void *func)
{
	*((ULONGLONG *)func) = 0;
}

#endif
}