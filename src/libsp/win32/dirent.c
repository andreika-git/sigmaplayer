#include <stdio.h>
#include <string.h>
#include "dirent.h"

struct _finddata_t first;
int wasfirst = 0;

DIR * opendir(const char *name)
{
	char tmp[4097], dir[4097];
	//int len;
	DIR *ret;
	wasfirst = 1;
	/*strcpy(tmp, name);
	len = strlen(tmp);
	if (tmp[len-1] != '/' && tmp[len-1] != '\\')
		strcat(tmp, "/");*/
	_getcwd(tmp, 1024);
	if (name[0] == '/')
	{
		strcpy(dir, tmp);
		strcat(dir, name);
		name = dir;
	}
	_chdir(name);
	ret = (DIR *)_findfirst("*.*", &first);
	_chdir(tmp);
	return ret;
}

int closedir (DIR *d)
{
	_findclose((long)d);
	return 0;
}

struct _finddata_t *readdir(DIR *d)
{
	if (wasfirst == 1)
	{
		wasfirst = 0;
		return &first;
	}
	if (_findnext((long)d, &first) != 0)
		return NULL;
	return &first;
}

int readlink(char *path, char *buf, size_t size)
{
	path = path;
	buf = buf;
	size = size;
	
	return -1;
}

