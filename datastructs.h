/* vim: set sw=4 sts=4 : */

/*
 * Author David Bruno Cortarello <Nomius>. Redistribute under the terms of the
 * BSD-lite license. Bugs, suggests, nor projects: dcortarello@gmail.com
 *
 * Program: kpkg
 * Version: 4.0a
 *
 *
 * Copyright (c) 2002-2010, David B. Cortarello
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice
 *    and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *  * Neither the name of Kwort nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


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

#define MAX_QUERY (PATH_MAX+40+80)

#define MIRRORS_DIRECTORY "/var/packages/mirrors"
#define PACKAGES_DIRECTORY "/var/packages/downloads"

#define PKG_NAME 40
#define PKG_VERSION 10
#define PKG_ARCH 10
#define PKG_BUILD 3
#define PKG_EXTENSION 10
#define PKG_CRC	15
#define PKG_COMMENT 80

/* Distribution databases */
/* CREATE TABLE PACKAGES(NAME TEXT KEY ASC, VERSION TEXT, ARCH TEXT, BUILD TEXT, EXTENSION TEXT, TEXT CRC); */
/* CREATE TABLE FILESPKG(NAME TEXT, FILENAME TEXT NOT NULL, CRC TEXT) */

/* Mirror database structures */
/* CREATE TABLE MIRRORPKG(NAME TEXT KEY ASC, VERSION TEXT, BUILD TEXT, LINK TEXT, COMMENT TEXT, CRC TEXT) */
/* CREATE TABLE MDATA (FIELD TEXT KEY, VALUE TEXT) */

typedef struct _PkgData {
	char name[PKG_NAME+1];
	char version[PKG_VERSION+1];
	char arch[PKG_ARCH+1];
	char build[PKG_BUILD+1];
	char extension[PKG_EXTENSION+1];
	char crc[PKG_CRC+1];
	int  files_num;
	char **files;
	char comment[PKG_COMMENT+1];
} PkgData;

typedef struct _myDir {
	char **directories;
	int index;
} myDir;

typedef struct _ListOfPackages {
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

