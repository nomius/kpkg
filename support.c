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
 * This is a support function to know if a given package name exists in the PACKAGES database. If the package exists in the database, its info is restored in the Data structure given
 * @param Data A data structure with the name of the package
 * @return If the package exists, 1 is returned, if it doesn't, 0 is returned. If an error ocurrs, -1 is returned (And an error message is issued).
 */
int ExistsPkg(PkgData *Data)
{
	char query[MAX_QUERY];
	char versionb[10];

	strncpy(versionb, Data->version, PKG_VERSION);
	memset(Data->version, '\0', sizeof(Data->version));

	snprintf(query, MAX_QUERY, "SELECT NAME, VERSION, ARCH, BUILD, EXTENSION FROM PACKAGES WHERE NAME = '%s'", Data->name);
	if (sqlite3_exec(Database, query, &ReturnSilentDataFromDB, Data, NULL)) {
		fprintf(stderr, "Couldn't search for [%s] in database [%s]\n", Data->name, dbname);
		return -1;
	}

	if (*Data->version == '\0') {
		strncpy(Data->version, versionb, PKG_VERSION);
		return 0;
	}

	return 1;
}

/**
 * This function transforms a package name (of the form "name-version-arch-build.extension) into a package common structure
 * @param Data A data structure where all the data will be stored in
 * @param filename A filename, only in its basename form
 * @return If the package is well formed, 0 is returned and the Data structure gets filled. Otherwise 1.
 */
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
