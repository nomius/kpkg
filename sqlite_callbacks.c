/* vim: set sw=4 sts=4 : */

/*
 * Author David Bruno Cortarello <Nomius>. Redistribute under the terms of the
 * BSD-lite license. Bugs, suggests, nor projects: dcortarello@gmail.com
 *
 * Program: kpkg
 * Version: 4.0a
 *
 *
 * Copyright (c) 2005-2008, David B. Cortarello
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
 * This function is the callback issued when a file is found for deletion. It removes the file from the database and then removes it from the disk
 * @param args A list of directories which will be needed to remove after (this list is updated in this function)
 * @param numCols The numbers of columns retrieved from the database
 * @param results The data retrieved of each column
 * @param columnNames The name of the columns retrieved
 * @return If the filename was removed, 0 is returned. In case of error, -1 (And an error message is issued)
 */
int RemoveFileCallback(void *args, int numCols, char **results, char **columnNames)
{
	char remove_query[MAX_QUERY], *name =  NULL, *filename = NULL;
	int i = 0;
	myDir *ptr = (myDir *)args;
	struct stat myfile;

	if (!strcmp(columnNames[0], "NAME")) {
		name = results[0];
		filename = results[1];
	}
	else {
		name = results[1];
		filename = results[0];
	}

	snprintf(remove_query, MAX_QUERY, "DELETE FROM FILESPKG WHERE NAME = '%s' AND FILENAME = '%s'", name , filename);

	if (sqlite3_exec(Database, remove_query, NULL, NULL, NULL)) {
		fprintf(stderr, "Couldn't remove file %s from database\n", filename);
		return -1;
	}

	if (stat(filename, &myfile))
		if (errno != ENOENT)
			fprintf(stderr, "Could not obtain the file information for %s (%s)\n", filename, strerror(errno));

	/* Save directories, so when all files are removed, we clean up those */
	if (S_ISDIR(myfile.st_mode)) {
		ptr->directories = realloc(ptr->directories, (ptr->index+1)*sizeof(char *));
		ptr->directories[ptr->index] = strdup(filename);
		ptr->index++;
	}
	else
		if (unlink(filename)) {
			fprintf(stderr, "Could not remove %s (%s)\n", filename, strerror(errno));
			return -1;
		}

	return 0;
}

/**
 * This function is the callback issued when a package link is found in any mirror database.
 * @param args A list of links (this list is updated in this function)
 * @param args The package name to wich this filename belongs to
 * @param numCols The numbers of columns retrieved from the database
 * @param results The data retrieved of each column
 * @param columnNames The name of the columns retrieved
 * @return Returns always 0 in order to keep the search until there's no packages in the database to check
 */
int SaveListOfLinks(void *args, int numCols, char **results, char **columnNames)
{
	ListOfLinks *ptr = (ListOfLinks *)args;
	char *link = NULL, *comment = NULL, *crc = NULL;
	int i = 0;


	for (i=0; i<numCols; i++){
		if (!strcmp(columnNames[i], "LINK"))
			link = results[i];
		if (!strcmp(columnNames[i], "COMMENT"))
			comment = results[i];
		if (!strcmp(columnNames[i], "CRC"))
			crc = results[i];
	}

	ptr->links = realloc(ptr->links, (ptr->index+1)*sizeof(char *));
	ptr->links[ptr->index] = strdup(link);
	ptr->comments = realloc(ptr->comments, (ptr->index+1)*sizeof(char *));
	ptr->comments[ptr->index] = strdup(comment);
	ptr->crcs = realloc(ptr->crcs, (ptr->index+1)*sizeof(char *));
	ptr->crcs[ptr->index] = strdup(crc);
	ptr->index++;

	return 0;
}

/**
 * This function is the callback issued when a package search is issued. It prints the search output in the stdout
 * @param args The package name to wich this filename belongs to
 * @param numCols The numbers of columns retrieved from the database
 * @param results The data retrieved of each column
 * @param columnNames The name of the columns retrieved
 * @return Returns always 0 in order to keep the search until there's no packages in the database to check
 */
int SearchPkgPrintCallback(void *args, int numCols, char **results, char **columnNames)
{
	int i = 0;
	int *found = (int *)args;
	char csv[75], *ecsv = NULL;
	PkgData Data;

	*found = 1;
	ecsv = getenv("CSV");

	for (i=0; i<numCols; i++){
		if (!strcmp(columnNames[i], "NAME")) {
			if (!ecsv)
				printf("Package name: %s\n", results[i]);
			else
				strncpy(Data.name, results[i], PKG_NAME);
		}
		if (!strcmp(columnNames[i], "VERSION")) {
			if (!ecsv)
				printf(" '-> Version: %s\n", results[i]);
			else
				strncpy(Data.version, results[i], PKG_VERSION);
		}
		if (!strcmp(columnNames[i], "ARCH")) {
			if (!ecsv)
				printf(" '->    Arch: %s\n", results[i]);
			else
				strncpy(Data.arch, results[i], PKG_ARCH);
		}
		if (!strcmp(columnNames[i], "BUILD")) {
			if (!ecsv)
				printf(" '->   Build: %s\n", results[i]);
			else
				strncpy(Data.build, results[i], PKG_BUILD);
		}
		if (!strcmp(columnNames[i], "EXTENSION")) {
			if (!ecsv)
				printf(" '->     Ext: %s\n", results[i]);
			else
				strncpy(Data.extension, results[i], PKG_EXTENSION);
		}
	}
	if (ecsv)
		printf("%s;%s;%s;%s;%s", Data.name, Data.version, Data.arch, Data.build, Data.extension);
	printf("\n");

	return 0;
}

/**
 * This function is the callback issued when a file search is issued. It prints the name of the package that provides a file by stdout
 * @param args The package name to wich this filename belongs to
 * @param numCols The numbers of columns retrieved from the database
 * @param results The data retrieved of each column
 * @param columnNames The name of the columns retrieved
 * @return This function returns always 1.
 */
int SearchFilePrintCallback(void *args, int numCols, char **results, char **columnNames)
{
	int i = 0;
	char *filename = NULL, *name = NULL;

	for (i=0; i<numCols; i++){
		if (!strcmp(columnNames[i], "NAME"))
			name = results[i];
		if (!strcmp(columnNames[i], "FILENAME"))
			filename = results[i];
	}
	printf("%s [%s] provided by %s\n", (char *)args, filename, name);

	return 0;
}

/**
 * This function is the callback issued by the ExistsPkg function. It retrieves the common package information from the database.
 * @param args The package name to wich this filename belongs to
 * @param numCols The numbers of columns retrieved from the database
 * @param results The data retrieved of each column
 * @param columnNames The name of the columns retrieved
 * @return This function returns always 0.
 */
int ReturnSilentDataFromDB(void *args, int numCols, char **results, char **columnNames)
{
	int i = 0;
	PkgData *ptr = (PkgData *)args;

	for (i=0; i<numCols; i++){
		if (!strcmp(columnNames[i], "NAME"))
			strncpy(ptr->name, results[i], PKG_NAME);
		if (!strcmp(columnNames[i], "VERSION"))
			strncpy(ptr->version, results[i], PKG_VERSION);
		if (!strcmp(columnNames[i], "ARCH"))
			strncpy(ptr->arch, results[i], PKG_ARCH);
		if (!strcmp(columnNames[i], "BUILD"))
			strncpy(ptr->build, results[i], PKG_BUILD);
		if (!strcmp(columnNames[i], "EXTENSION"))
			strncpy(ptr->extension, results[i], PKG_EXTENSION);
	}

	return 0;
}

/**
 * This function is the callback issued by the GetDataFromMirrorDatabase function. It retrieves the field asked by the caller
 * @param args The field where the data is going to be saved
 * @param numCols The numbers of columns retrieved from the database
 * @param results The data retrieved of each column
 * @param columnNames The name of the columns retrieved
 * @return This function returns always 1 as only one field is enough
 */
int GetFieldCallback(void *args, int numCols, char **results, char **columnNames)
{
	char *value = (char *)args;

	strncpy(args, strdup(results[0]), PATH_MAX);

	return 0;
}

/**
 * This function is the callback issued by the GetListOfPackages function. It saves the database data in the ListOfPackages structure given
 * @param args The field where the data is going to be saved
 * @param numCols The numbers of columns retrieved from the database
 * @param results The data retrieved of each column
 * @param columnNames The name of the columns retrieved
 * @return This function returns always 1 as only one field is enough
 */
int SavePackageListCallback(void *args, int numCols, char **results, char **columnNames)
{
	int i = 0;
	ListOfPackages *ptr = (ListOfPackages *)args;

	ptr->packages = realloc(ptr->packages, (ptr->index+1)*sizeof(char *));
	ptr->packages = realloc(ptr->versions, (ptr->index+1)*sizeof(char *));
	ptr->packages = realloc(ptr->builds, (ptr->index+1)*sizeof(char *));
	for (i=0; i<numCols; i++) {
		if (!strcmp(columnNames[i], "NAME"))
			ptr->packages[ptr->index] = strdup(results[i]);
		if (!strcmp(columnNames[i], "VERSION"))
			ptr->versions[ptr->index] = strdup(results[i]);
		if (!strcmp(columnNames[i], "BUILD"))
			ptr->builds[ptr->index] = strdup(results[i]);
	}
	ptr->index++;

	return 0;
}

