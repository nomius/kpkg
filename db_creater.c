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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sqlite3.h>
#include <zlib.h>

#define PKG_NAME 40
#define PKG_VERSION 10
#define PKG_ARCH 10
#define PKG_BUILD 3
#define PKG_EXTENSION 10
#define PKG_CRC 15
#define MAX_QUERY 1024

typedef struct _PkgData {
	char name[PKG_NAME+1];
	char version[PKG_VERSION+1];
	char arch[PKG_ARCH+1];
	char build[PKG_BUILD+1];
	char extension[PKG_EXTENSION+1];
	char crc[PKG_CRC+1];
	char link[PATH_MAX];
} PkgData;

char *link_pkgs;

int GiveMeHash(char *filename, char *hash)
{
	int fd = 0, i = 0;
	char data[8192];
	uLong crc;

	/* Initialize the crc */
	crc = crc32(0L, NULL, 0);

	/* Open the file */
	if ((fd = open(filename, O_RDONLY)) == -1) {
		fprintf(stderr, "Couldn't open %s (%s)\n", filename, strerror(errno));
		return -1;
	}

	/* Create the new hash */
	while((i = read(fd, data, sizeof(data))) > 0)
		crc = crc32(crc, (const Bytef *)data, i);
	close(fd);

	sprintf(hash, "%lu", crc);

	return 0;
}

int FillPkgDataFromPackage(PkgData *Data, char *filename)
{
	int i = 0, j = 0, end = 0;

	i = end = strlen(filename);

	/* Get build and extension */
	for (;filename[i] != '-' && i>0;i--) ;
	if (i == 0) return 1;
	i++;
	/* While we are not on a ., it is part of the build */
	for (j=i;filename[j]!='.' && j<end; j++) ;
	if (j == end || j == i) return 1;
	strncpy(Data->build, filename+i, j-i);
	/* From the . to the end we are in the extension */
	strncpy(Data->extension, filename+j+1, end-j);
	/* Now, save the arch */
	i -= 2;
	end = i;
	for (;filename[i]!= '-' && i>0;i--);
	if (i==0 || i == end) return 1;
	j = i-1;
	i++;
	strncpy(Data->arch, filename+i, end-i+1);
	/* Now, save the version */
	i -= 2;
	end = i;
	for (;filename[i]!= '-' && i>0;i--);
	if (i==0) return 1;
	j = i-1;
	i++;
	strncpy(Data->version, filename+i, end-i+1);
	/* Now save the name */
	if (j == 0) return 1;
	strncpy(Data->name, filename, j+1);

	/* The link */
	sprintf(Data->link, "%s/%s", link_pkgs, filename);

	/* Get the hash */
	if (GiveMeHash(filename, Data->crc)) {
		fprintf(stderr, "Error aquiring crc32 from file [%s]\n", filename);
		return 1;
	}
	return 0;
}

void usage(st)
{
	fprintf(stdout, "Usage: db_creater <DB NAME> <DB LINK> <DB DESC> <LINKS PKG>\n");
	if (st != -1)
		exit(st);
}


int main(int argc, char *argv[])
{
	DIR *dip;
	struct dirent *dit;
	char query[MAX_QUERY];
	sqlite3 *Database;
	PkgData pkg;

	if (argc != 5)
		usage(1);

	char *dbname    = argv[1];
	char *dblink    = argv[2];
	char *dbdesc    = argv[3];

	link_pkgs = argv[4];

	/* Get the databases from the directories */
	if ((dip = opendir(".")) == NULL) {
		fprintf(stderr, "Couldn't open directory . (%s)\n", strerror(errno));
		return -1;
	}

	/* Let's open the database */
	if (sqlite3_open(dbname, &Database)) {
		fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
		return -1;
	}

	/* Create the packages table */
	sprintf(query, "CREATE TABLE MIRRORPKG(NAME TEXT KEY ASC, VERSION TEXT, ARCH TEXT, BUILD TEXT, EXTENSION TEXT, LINK TEXT, CRC TEXT, COMMENT TEXT)");
	if (sqlite3_exec(Database, query, NULL, NULL, NULL)) {
		fprintf(stderr, "Couldn't create table MIRRORPKG in %s\n", dbname);
		return -1;
	}

	/* Create the mirrror data's table */
	sprintf(query, "CREATE TABLE MDATA (FIELD TEXT KEY, VALUE TEXT)");
	if (sqlite3_exec(Database, query, NULL, NULL, NULL)) {
		fprintf(stderr, "Couldn't create table MDATA in %s\n", dbname);
		return -1;
	}

	/* Insert the link into the MDATA table */
	sprintf(query, "INSERT INTO MDATA (FIELD, VALUE) values ('LINK', '%s')", dblink);
	if (sqlite3_exec(Database, query, NULL, NULL, NULL)) {
		fprintf(stderr, "Couldn't store the link in table MTABLE in %s\n", dbname);
		return -1;
	}

	/* Insert the description into the MDATA table */
	sprintf(query, "INSERT INTO MDATA (FIELD, VALUE) values ('DESC', '%s')", dbdesc);
	if (sqlite3_exec(Database, query, NULL, NULL, NULL)) {
		fprintf(stderr, "Couldn't store the description in table MTABLE in %s\n", dbname);
		return -1;
	}

	while ((dit = readdir(dip)) != NULL) {
		/* Skip directories . and .. */
		if (!strcmp(dit->d_name, ".") || !strcmp(dit->d_name, ".."))
			continue;

		memset(&pkg, '\0', sizeof(PkgData));
		if (FillPkgDataFromPackage(&pkg, dit->d_name)) {
			fprintf(stderr, "Wrong package name [%s]\n", dit->d_name);
			continue;
		}

		snprintf(query, MAX_QUERY, "INSERT INTO MIRRORPKG (NAME, VERSION, ARCH, BUILD, EXTENSION, LINK, CRC) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s')", 
				pkg.name, pkg.version, pkg.arch, pkg.build, pkg.extension, pkg.link, pkg.crc);

		if (sqlite3_exec(Database, query, NULL, NULL, NULL)) {
			fprintf(stderr, "Couldn't store the package %s with version %s in %s\n", pkg.name, pkg.version, dbname);
			return -1;
		}
	}
	closedir(dip);
	sqlite3_close(Database);

	return 0;
}
