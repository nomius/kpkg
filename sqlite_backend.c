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


#include "datastructs.h"

/**
 * This function removes the common data package from the PACKAGES table
 * @param name A package name
 * @return If no error ocurr, 0 is returned, otherwise -1 (And an error message is issued by stderr).
 */
int RemovePkgData(char *name)
{
    char query[MAX_QUERY];

    snprintf(query, MAX_QUERY, "DELETE FROM PACKAGES WHERE NAME = '%s'", name);
    if (sqlite3_exec(Database, query, NULL, NULL, NULL)) {
        fprintf(stderr, "Couldn't remove the package %s from the database %s\n", name, dbname);
        return -1;
    }
    return 0;
}


/**
 * This function removes all files entries in the FILESPKG table from a given package name
 * @param name A package name
 * @return If no error ocurr, 0 is returned, otherwise -1 (And an error message is issued by stderr).
 */
int RemovePkgFiles(char *name)
{
    char query[MAX_QUERY];
	myDir dirs;
	int i = 0;

	dirs.directories = NULL;
	dirs.index = 0;

	snprintf(query, MAX_QUERY, "SELECT NAME, FILENAME FROM FILESPKG WHERE NAME = '%s'", name);
	if (sqlite3_exec(Database, query, &RemoveFileCallback, &dirs, NULL))
		return -1;

	i = dirs.index;
	/* Remove empty directories */
	dirs.index--;
	while(dirs.index >= 0) {
		if (rmdir(dirs.directories[dirs.index]))
			if (errno != ENOTEMPTY)
				fprintf(stderr, "Could not remove directory %s (%s)\n", dirs.directories[dirs.index], strerror(errno));
		free(dirs.directories[dirs.index]);
		dirs.index--;
	}

	dirs.index = i;
	free(dirs.directories);
    return 0;
}


/**
 * This function searchs for a package in the PACKAGES database. It has to be improved to support regex instead of the "LIKE" operator
 * @param name A package name
 * @return If the package doesn't exists, 0 is returned, otherwise 1. In case of error -1 is returned (And an error message is issued).
 */
int SearchPkg(char *name)
{
	int found = 0;
	DIR *dip;
	struct dirent *dit;
	char tmp[MAX_QUERY];
	sqlite3 *TMPDatabase;

	if (sqlite3_open(dbname, &Database)) {
		fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
		return -1;
	}

	if (!strcmp(name, "/all"))
		snprintf(tmp, MAX_QUERY, "SELECT NAME, VERSION, ARCH, BUILD, EXTENSION, DATE FROM PACKAGES");
	else
		snprintf(tmp, MAX_QUERY, "SELECT NAME, VERSION, ARCH, BUILD, EXTENSION, DATE FROM PACKAGES WHERE NAME LIKE '%%%s%%'", name);

	if (sqlite3_exec(Database, tmp, &SearchPkgPrintCallback, &found, NULL)) {
		fprintf(stderr, "Couldn't search for %s (%s)\n", name, sqlite3_errmsg(Database));
		sqlite3_close(Database);
		return -1;
	}
	sqlite3_close(Database);

	if ((dip = opendir(MIRRORS_DIRECTORY)) == NULL) {
		fprintf(stderr, "Couldn't open the mirror's database directory %s (%s)\n", MIRRORS_DIRECTORY, strerror(errno));
		return -1;
	}

	found = 2;
	while ((dit = readdir(dip)) != NULL) {
		if (!strcmp(dit->d_name, ".") || !strcmp(dit->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", MIRRORS_DIRECTORY, dit->d_name);
		if (sqlite3_open(tmp, &TMPDatabase)) {
			fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
			closedir(dip);
			return -1;
		}
		snprintf(tmp, MAX_QUERY, "SELECT NAME, VERSION, ARCH, BUILD, EXTENSION, COMMENT FROM MIRRORPKG WHERE NAME LIKE '%%%s%%'", name);
		if (sqlite3_exec(TMPDatabase, tmp, &SearchPkgPrintCallback, &found, NULL)) {
			sqlite3_close(Database);
			closedir(dip);
			return -1;
		}
		sqlite3_close(TMPDatabase);
	}
	closedir(dip);

	if (!found)
		fprintf(stdout, "Package %s does not exists in database\n", name);

	return (found == 0 && found == 2)?0:1;
}


/**
 * This function searchs for a given filename in the FILESPKG database. It has to be improved to support regex instead of the "LIKE" operator
 * @param filename A filename in the database to search for
 * @return If no error ocurr, 0 is returned. Otherwise -1 is returned (And an error message is issued).
 */
int SearchFileInPkgDB(char *filename)
{
	char query[MAX_QUERY];

	if (sqlite3_open(dbname, &Database)) {
		fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
		return -1;
	}

	/*snprintf(query, MAX_QUERY, "SELECT NAME FROM FILESPKG WHERE FILENAME REGEXP '^%s$|/%s$|^%s/|/%s/'", filename, filename, filename, filename);*/
	snprintf(query, MAX_QUERY, "SELECT NAME, FILENAME FROM FILESPKG WHERE FILENAME LIKE '%%%s%%'", filename);
	if (sqlite3_exec(Database, query, &SearchFilePrintCallback, filename, NULL)) {
		fprintf(stderr, "Couldn't search for [%s] in database [%s]\n", filename, dbname);
		sqlite3_close(Database);
		return -1;
	}
	sqlite3_close(Database);

	return 0;
}


/**
 * This function inserts the common data package in the PACKAGES table
 * @param Data The common data structure
 * @return If no error ocurr, 0 is returned, otherwise -1 (And an error message is issued by stderr).
 */
static int InsertPkgData(PkgData *Data)
{
	char query[MAX_QUERY];

	snprintf(query, MAX_QUERY, "INSERT INTO PACKAGES (NAME, VERSION, ARCH, BUILD, EXTENSION, CRC, DATE) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s')", Data->name, Data->version, Data->arch, Data->build, Data->extension, Data->crc, Data->date);
	if (sqlite3_exec(Database, query, NULL, NULL, NULL)) {
		fprintf(stderr, "Couldn't store the package %s with version %s in %s\n", Data->name, Data->version, dbname);
		return -1;
	}

	return 0;
}


/**
 * This function inserts a filename given in FILESPKG table associated to a package
 * @param name The package name
 * @param filename The filename to be added to the table FILESPKG
 * @param crc A checksum for the file (Now it's not being used)
 * @return If no error ocurr, 0 is returned, otherwise -1 (And an error message is issued by stderr).
 */
static int InsertPkgFile(char *name, char **filenames, char *crc, int fileCount)
{
	int i = 0;
	sqlite3_stmt *stmt;

	sqlite3_exec(Database, "BEGIN TRANSACTION", NULL, NULL, NULL);

	for (i = 0; i < fileCount; i++) {
		if (strcmp(filenames[i], "") == 0) {
			continue;
		}

		if (sqlite3_prepare(Database, "INSERT INTO FILESPKG (NAME, FILENAME, CRC) VALUES (?,?,?)", -1, &stmt, 0) != SQLITE_OK) {
			fprintf(stderr, "Could not prepare statement");
			return -1;
		}

		if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK) {
			fprintf(stderr, "Could not bind first parameter");
			return -1;
		}

		if (sqlite3_bind_text(stmt, 2, filenames[i], -1, SQLITE_STATIC) != SQLITE_OK) {
			fprintf(stderr, "Could not bind second parameter");
			return -1;
		}

		if (sqlite3_bind_text(stmt, 3, crc, -1, SQLITE_STATIC) != SQLITE_OK) {
			fprintf(stderr, "Could not bind third parameter");
			return -1;
		}

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			sqlite3_reset(stmt);
			fprintf(stderr, "Could not execute prepared statement");
		}
	}

	sqlite3_exec(Database, "COMMIT TRANSACTION", NULL, NULL, NULL);
	sqlite3_reset(stmt);
	
	return 0;
}


/**
 * This function inserts the whole package structure in the PACKAGES and FILESPKG tables calling to InsertPkgData and InsertPkgFile respectively
 * @param Data A package data structure
 * @return If no error ocurr, 0 is returned. If an error ocurr while inserting the common data package -1 is returned. Otherwise it returns the number of files that failed in the insertion in FILESPKG
 * */
int InsertPkgDB(PkgData *Data)
{
	if (InsertPkgData(Data) == -1)
		return -1;

	if (InsertPkgFile(Data->name, Data->files, NULL, Data->files_num) == -1)
		return -1;

	return 0;
}


/**
 * This function search for links in the mirror's databases
 * @param Data A package data structure
 * @return If no error ocurr, 0 is returned. If an error ocurr while inserting the common data package -1 is returned. Otherwise it returns the number of files that failed in the insertion in FILESPKG
 */
int SearchLinkForPackage(char *name, ListOfLinks *Links, char *MIRROR)
{
	DIR *dip = NULL;
	struct dirent *dit = NULL;
	char tmp[MAX_QUERY];
	sqlite3 *TMPDatabase = NULL;

	if ((dip = opendir(MIRRORS_DIRECTORY)) == NULL) {
		fprintf(stderr, "Couldn't open the mirror's database directory %s (%s)\n", MIRRORS_DIRECTORY, strerror(errno));
		return -1;
	}

	while ((dit = readdir(dip)) != NULL) {
		if (!strcmp(dit->d_name, ".") || !strcmp(dit->d_name, ".."))
			continue;
		if (MIRROR && strcmp(MIRROR, dit->d_name))
			continue;
		sprintf(tmp, "%s/%s", MIRRORS_DIRECTORY, dit->d_name);
		if (sqlite3_open(tmp, &TMPDatabase)) {
			fprintf(stderr, "Failed to open database %s (%s)\n", tmp, sqlite3_errmsg(TMPDatabase));
			closedir(dip);
			return -1;
		}
		snprintf(tmp, MAX_QUERY, "SELECT LINK, COMMENT, CRC FROM MIRRORPKG WHERE NAME = '%s'", name);
		if (sqlite3_exec(TMPDatabase, tmp, &SaveListOfLinks, Links, NULL)) {
			sqlite3_close(TMPDatabase);
			closedir(dip);
			return -1;
		}
		if (sqlite3_close(TMPDatabase) != SQLITE_OK) {
			fprintf(stderr, "Failed to close database %s (%s)\n", tmp, sqlite3_errmsg(TMPDatabase));
			return -1;
		}
	}
	closedir(dip);
	if (Links->index == 0) {
		if (MIRROR)
			fprintf(stdout, "There's no links for package %s in mirror %s\n", name, MIRROR);
		else
			fprintf(stdout, "There's no links for package %s\n", name);
		return 1;
	}

	return 0;
}


/**
 * This function is a generic function to get data (according to the field given) from MDATA table in the mirror's database
 * @param db A string giving the sqlite3 database to be used
 * @param field The field that you want to filter
 * @param value The string where you want the returned data to be saved
 * @return If no error ocurr, 0 is returned. If an error ocurr -1 is returned (and an error message is issued).
 */
int GetDataFromMirrorDatabase(char *db, char *field, char *value)
{
	sqlite3 *TMPDatabase;
	char query[MAX_QUERY];

	if (sqlite3_open(db, &TMPDatabase)) {
		fprintf(stderr, "Failed to open database %s (%s)\n", db, sqlite3_errmsg(TMPDatabase));
		return -1;
	}

	snprintf(query, MAX_QUERY, "SELECT VALUE FROM MDATA WHERE FIELD = '%s'", field);
	if (sqlite3_exec(TMPDatabase, query, &GetFieldCallback, value, NULL)) {
		fprintf(stderr, "Couldn't search for %s (%s)\n", field, sqlite3_errmsg(Database));
		sqlite3_close(TMPDatabase);
		return -1;
	}
	sqlite3_close(TMPDatabase);

	return 0;
}

/**
 * This function provides a list of the installed packages in the system
 * @param Packages A pointer to the list of packages that will be filled with data
 * @return If no error ocurr, 0 is returned. If an error ocurr -1 is returned (and an error message is issued).
 */
int GetListOfPackages(ListOfPackages *Packages)
{
	char query[MAX_QUERY];

	Packages->index    = 0;
	Packages->packages = NULL;
	Packages->versions = NULL;
	Packages->builds   = NULL;

	if (sqlite3_open(dbname, &Database)) {
		fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
		return -1;
	}

	snprintf(query, MAX_QUERY, "SELECT NAME, VERSION, BUILD FROM PACKAGES");
	if (sqlite3_exec(Database, query, &SavePackageListCallback, Packages, NULL)) {
		fprintf(stderr, "Couldn't get the list of installed packages (%s)\n", sqlite3_errmsg(Database));
		sqlite3_close(Database);
		return -1;
	}
	sqlite3_close(Database);

	return 0;
}

int NewVersionAvailable(PkgData *Data, char *MIRROR)
{
	DIR *dip;
	struct dirent *dit;
	char tmp[MAX_QUERY];
	sqlite3 *TMPDatabase;
	PkgData TMPData;

	MIRROR[0] = '\0';
	if ((dip = opendir(MIRRORS_DIRECTORY)) == NULL) {
		fprintf(stderr, "Couldn't open the mirror's database directory %s (%s)\n", MIRRORS_DIRECTORY, strerror(errno));
		return -1;
	}

	while ((dit = readdir(dip)) != NULL) {
		if (!strcmp(dit->d_name, ".") || !strcmp(dit->d_name, ".."))
			continue;
		sprintf(tmp, "%s/%s", MIRRORS_DIRECTORY, dit->d_name);
		if (sqlite3_open(tmp, &TMPDatabase)) {
			fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
			closedir(dip);
			return -1;
		}
		memset(&TMPData, '\0', sizeof(PkgData));
		snprintf(tmp, MAX_QUERY, "SELECT NAME, VERSION, BUILD FROM MIRRORPKG WHERE NAME = '%s'", Data->name);
		if (sqlite3_exec(TMPDatabase, tmp, &ReturnSilentDataFromDB, &TMPData, NULL)) {
			fprintf(stderr, "Couldn't check if the package is in the mirror's database (%s)\n", sqlite3_errmsg(TMPDatabase));
			sqlite3_close(TMPDatabase);
			closedir(dip);
			return -1;
		}
		sqlite3_close(TMPDatabase);
		/* Return the mirror if the version or builds found are differents */
		if (TMPData.version[0] != '\0' && TMPData.build[0] != '\0' && (strcmp(TMPData.version, Data->version) || strcmp(TMPData.build, Data->build))) {
			strncpy(MIRROR, dit->d_name, NAME_MAX);
			sqlite3_close(TMPDatabase);
			closedir(dip);
			return 1;
		}
		sqlite3_close(TMPDatabase);
	}
	closedir(dip);

	return 0;
}

