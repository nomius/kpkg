/* vim: set sw=4 sts=4 */

/*
 * Author David Bruno Cortarello <Nomius>. Redistribute under the terms of the
 * BSD-lite license. Bugs, suggests, nor projects: dcortarello@gmail.com
 *
 * Program: kpkg
 * Version: 4.0
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

#ifndef _DATASTRUCTS_H
#define _DATASTRUCTS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <libgen.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <zlib.h>
#include <archive.h>
#include <archive_entry.h>
#include <time.h>

#include "sqlite_callbacks.h"

#define MAX_QUERY (PATH_MAX+40+80)

#define PKG_NAME 64
#define PKG_VERSION 40
#define PKG_ARCH 20
#define PKG_BUILD 20
#define PKG_EXTENSION 20
#define PKG_CRC	40
#define PKG_COMMENT 80
#define PKG_DATE 40

#define INSTALL "install"
#define DOINST_FILE (INSTALL "/doinst.sh")
#define README_FILE (INSTALL "/README")

#define PACKAGE_NAME_FORMAT "name#version#arch#build.ext"

#define KPKG_DB_HOME_DEFAULT "/var/packages/installed.kdb"
#define MIRRORS_DIRECTORY_DEFAULT "/var/packages/mirrors"
#define PACKAGES_DIRECTORY_DEFAULT "/var/packages/downloads"

#define EXCEPTIONS_FILE "/etc/kpkg.exceptions"
#define INIT 0
#define DEFAULT 1

/* Distribution databases */
/* CREATE TABLE PACKAGES(NAME TEXT KEY ASC, VERSION TEXT, ARCH TEXT, BUILD TEXT, EXTENSION TEXT, TEXT CRC, TEXT DATE); */
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
	char date[PKG_DATE+1];
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
	char **name;
	char **version;
	char **arch;
	char **build;
	char **extension;
	char **links;
	char **comments;
	char **crcs;
	int index;
} ListOfLinks;

typedef struct _ResultFound {
	int found;
	char mirror_name[NAME_MAX];
} ResultFound;

char *dbname;
sqlite3 *Database;
char *HOME_ROOT;
char *MIRRORS_DIRECTORY;
char *PACKAGES_DIRECTORY;
int noreadme;
int skip;
int force;
int local_only;
char **Exceptions;
int TotalExceptions;

#endif
