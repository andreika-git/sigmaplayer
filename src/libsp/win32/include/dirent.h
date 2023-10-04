#include <io.h>
#include <direct.h>

#ifndef __DIRENT_H__
#define __DIRENT_H__

#define DIR int
#define PATH_MAX        4096

#define d_name name
#define dirent _finddata_t

#define S_IROTH _S_IREAD
#define S_IRGRP 0
#define S_IRWXU 0
#define	S_IRUSR	0 //	0000400
#define	S_IWUSR	0 //	0000200
#define	S_IXUSR	0 //	0000100
#define	S_IRWXG	0 //	0000070
#define	S_IWGRP	0 //	0000020
#define	S_IXGRP	0 //	0000010
#define	S_IRWXO	0 //	0000007
#define	S_IWOTH	0 //	0000002
#define	S_IXOTH	0 //	0000001

#define	S_IFBLK	0060000

#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define lstat stat
#define stat64 _stati64

#define STAT_INODE(X) (X.st_size * X.st_mtime * X.st_ctime)

typedef int mode_t;

#ifdef	__cplusplus
extern "C"
{
#endif

DIR * opendir(const char *name);
int closedir (DIR *);

struct _finddata_t *readdir(DIR *);

int readlink(char *path, char *buf, size_t size);

#ifdef	__cplusplus
}
#endif

#endif // of __DIRENT_H__