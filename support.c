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
#include "support.h"

static void GetSysDate(char *out)
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(out, 20,"%Y/%m/%d %I:%M:%S", timeinfo);
}


/**
 * This function transforms a package name (of the form "name#version#arch#build.extension) into a package common structure
 * @param Data A data structure where all the data will be stored in
 * @param filename A filename, only in its basename form
 * @return If the package is well formed, 0 is returned and the Data structure gets filled. Otherwise 1.
 * TODO: Improve this function to get rid of char *fields
 */
int FillPkgDataFromPackage(PkgData *Data, char *filename)
{
	char tmp[PKG_BUILD+1+PKG_EXTENSION+1], *tstr = NULL, tmp2[PATH_MAX];
	char *fields[] = { Data->name, Data->version, Data->arch, tmp};
	register int i = 0;

	strncpy(tmp2, filename, PATH_MAX);

	for (tstr = strtok(tmp2, "#"); i <= 3; tstr = strtok(NULL, "#"), i++) {
		if (!tstr)
			return 1;
		strcpy(fields[i], tstr);
	}

	if ((tstr = strchr(tmp, '.'))) {
		strncpy(Data->build, tmp, tstr-tmp);
		strncpy(Data->extension, tstr+1, PKG_EXTENSION-1);
	}
	else
		return 1;

	Data->extension[strlen(Data->extension)] = '\0';
	GetSysDate(Data->date);

	if (Data->name[0] == '\0' || Data->version[0] == '\0' || Data->arch[0] == '\0' || Data->build[0] == '\0' || Data->extension[0] == '\0')
		return 1;

    return 0;
}

/**
 * This function is issed to free the links of the ListOfLinks structure
 * @param Links The list of links to be freed
 */
void freeLinks(ListOfLinks *Links)
{
	int i = 0;

	for (i = 0; i < Links->index; i++) {
		free(Links->links[i]);
		free(Links->comments[i]);
		free(Links->crcs[i]);
	}
	free(Links->links);
	free(Links->comments);
	free(Links->crcs);
}


/**
 * Given a List of Links this function is a user interaction function to get the number of link required
 * @param Links The list of links to iterate
 * @return a valid index in the array
 */
int GetLinkFromInput(ListOfLinks *Links)
{
	int i = 0, s = 0;
	char *input = NULL;

	/* Print out all the links with their associated numbers */
	while (i < Links->index) {
		fprintf(stdout, "[%d] %s\n", i, Links->links[i]);
		i++;
	}
	i = 0;

	/* Input to get the link */
	while (1) {
		/* EOF leaves */
		printf("Which one do you want to download? [q (cancel) / c (show comment) / number]: ");
		s = getline(&input, (size_t *)&s, stdin);
		if (*input == EOF || *input == '\n' || s == 0 || s == 1 || s == -1) {
			free(input);
			freeLinks(Links);
			return 1;
		}
		/* q leaves leaves */
		else if (*input == 'q') {
			freeLinks(Links);
			free(input);
			return 1;
		}
		/* input the "c" letter and its number to get the link comments */
		else if (*input == 'c') {
			i = atoi(input + 1);
			if (i < Links->index && i >= 0)
				fprintf(stdout, "Comments for [%d]:\n%s\n", i, Links->comments[i]);
			else
				fprintf(stdout, "Invalid link number\n");
			free(input);
		}
		/* If no of the cases above and it is not a number, then prompt again */
		else if (!isdigit(*input)) {
			fprintf(stdout, "Invalid character\n");
			free(input);
			continue;
		}
		/* If it is a number get that number as it is OUR number */
		else {
			i = atoi(input);
			if (i < Links->index && i >= 0) {
				free(input);
				break;
			}
			fprintf(stdout, "Invalid link given\n");
		}
	}
	return i;
}


/**
 * This function gets the full path of a given file
 * @param name A filename
 * @param pkgfullpathname The full path (output)
 * @return 0
 */
int GetFileFullPath(char *package, char *pkgfullpathname)
{
	char *ptr_name = basename(package);
	char *pkgfullpath = dirname(package);
	char tmp[PATH_MAX];

	chdir(pkgfullpath);
	if (!(pkgfullpath = getcwd(tmp, PATH_MAX))) {
		fprintf(stderr, "Can't get current path (%s)\n", strerror(errno));
		return -1;
	}
	snprintf(pkgfullpathname, PATH_MAX, "%s/%s", pkgfullpath, ptr_name);
	return 0;
}


#define FillField(x, len) \
	if (x) { \
		strncpy(out->x, x, len); \
		out->x[strlen(x)>len?len:strlen(x)] = '\0'; \
	}

/**
 * This function fills a PkgData structure from the given parameters
 * @param name a package name
 * @param version a package version
 * @param arch a package architecture
 * @param build a package build
 * @param extension a package extension
 * @param crc a package crc32
 * @param date a package installed date
 * @param comment a package comment
 */
void FillPkgDataStruct(PkgData *out, char *name, char *version, char *arch, char *build, char *extension, char *crc, char *date, char *comment)
{
	FillField(name, PKG_NAME)
	FillField(version, PKG_VERSION)
	FillField(arch, PKG_ARCH)
	FillField(build, PKG_BUILD)
	FillField(extension, PKG_EXTENSION)
	FillField(crc, PKG_CRC)
	FillField(date, PKG_DATE)
	FillField(comment, PKG_COMMENT)
}

/**
 * This function removes the extension of a given filename
 * @param s the filename
 * @return the string without the extension
 */
char *RemoveExtension(char *s)
{
	char *ptr = strrchr(s, '.');
	if (ptr != NULL)
		*ptr = '\0';
	return s;
}

/**
 * This function removes the current opened file when handling SIGINT or SIGTERM
 * @param sig the signal number
 */
void cleanup(int sig)
{
	if (*fileremove)
		unlink(fileremove);
	exit(1);
}
