/* vim: set sw=4 sts=4 : */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sqlite3.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <zlib.h>
#include <archive.h>
#include <archive_entry.h>
#include "sqlite_callbacks.h"

#ifndef PATH_MAX
	#define PATH_MAX 1024
#endif
#define MAX_QUERY (PATH_MAX+40+80)

#define MIRRORS_DIRECTORY "/var/packages/mirrors"
#define PACKAGES_DIRECTORY "/var/packages/downloads"

/* Distribution databases */
/* CREATE TABLE PACKAGES(NAME TEXT KEY ASC, VERSION TEXT, ARCH TEXT, BUILD TEXT, EXTENSION TEXT, TEXT CRC); */
/* CREATE TABLE FILESPKG(NAME TEXT, FILENAME TEXT NOT NULL, CRC TEXT) */

/* Mirror database structures */
/* CREATE TABLE MIRRORPKG(NAME TEXT KEY ASC, VERSION TEXT, BUILD TEXT, LINK TEXT, COMMENT TEXT, CRC TEXT) */
/* CREATE TABLE MDATA (FIELD TEXT KEY, VALUE TEXT) */

typedef struct _PkgData {
	char name[40];
	char version[10];
	char arch[10];
	char build[3];
	char extension[10];
	char crc[32];
	int  files_num;
	char **files;
} PkgData;

typedef struct _myDir {
	char **directories;
	int index;
} myDir;

typedef struct _ListOfDirectories {
	char **packages;
	char **versions;
	char **builds;
	int index;
} ListOfPackages;

typedef struct _ListOfLinks {
	char **links;
	char **comments;
	char **crcs;
	int index;
} ListOfLinks;

char *dbname;
sqlite3 *Database;
char *HOME_ROOT;

