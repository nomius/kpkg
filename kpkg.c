/* vim: set sw=4 sts=4 : */

/*
 * Author David Bruno Cortarello <Nomius>. Redistribute under the terms of the
 * BSD-lite license. Bugs, suggests, nor projects: dcortarello@gmail.com
 *
 * Program: kpkg
 * Version: 3.0e
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
 * This function removes the whole package structure from PACKAGES and FILESPKG tables
 * @param name A package name
 * @return If no error ocurr, 0 is returned. If the package doesn't exists, 1 is returned (And a message is issued). In case of error, -1 (And an error message is issued)
 */
int RemovePkg(char *name, int silent)
{
	char *pathb = getcwd(malloc(PATH_MAX), PATH_MAX);
	int ret = 0;
	PkgData Data;

	if (sqlite3_open(dbname, &Database)) {
		fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
		return -1;
	}

	if (chdir(HOME_ROOT)) {
		fprintf(stderr, "Can't go to %s (%s)\n", HOME_ROOT, strerror(errno));
		free(pathb);
		return -1;
	}

	memset(&Data, '\0', sizeof(Data));
	strcpy(Data.name, name);

	if (!ExistsPkg(&Data)) {
		if (silent)
			fprintf(stderr, "Package %s does not exists\n", name);
		chdir(pathb);
		free(pathb);
		return 1;
	}

	if (RemovePkgFiles(name) == -1) {
		chdir(pathb);
		free(pathb);
		return -1;
	}

	ret = RemovePkgData(name);
	sqlite3_close(Database);
	chdir(pathb);
	free(pathb);
	fprintf(stdout, "Package %s removed\n", name);
	return ret;
}

/**
 * This function installs a package calling ExtractPackage to extract it and InsertPkgDB to insert the package in PACKAGES and FILESPKG tables
 * @param name A package name
 * @return If no error ocurr, 0 is returned. If the package is already installed, 1 is returned (And a message is issued). In case of error, -1 (And an error message is issued)
 */
int InstallPkg(char *package)
{
	char *ptr_name = NULL, *renv = NULL, *pkgfullpath = NULL, *init_path = NULL;
	char pkgfullpathname[PATH_MAX];
	char output[PATH_MAX];
	char PackageOrig[PATH_MAX];
	int fd = 0;
	PkgData Data;

	strncpy(PackageOrig, package, PATH_MAX);
	init_path = getcwd(malloc(PATH_MAX), PATH_MAX);
	memset(&Data, '\0', sizeof(PkgData));

	if ((fd = open(package, O_RDONLY)) < 0) {
		/* Ok, the package doesn't exists, let's go for a mirror */
		if (DownloadPkg(package, output) == -1)
			return -1;
		package = output;
	}
	else 
		close(fd);
	ptr_name = (char *)basename(package);
	pkgfullpath = (char *)dirname(package);

	chdir(pkgfullpath);
	pkgfullpath = getcwd(malloc(PATH_MAX), PATH_MAX);
	snprintf(pkgfullpathname, PATH_MAX, "%s/%s", pkgfullpath, ptr_name);
	free(pkgfullpath);

	if (chdir(HOME_ROOT)) {
		fprintf(stderr, "Can't go to %s (%s)\n", HOME_ROOT, strerror(errno));
		free(init_path);
		return -1;
	}

	if (FillPkgDataFromPackage(&Data, ptr_name)) {
		fprintf(stdout, "Wrong package name format. It should be \"name-version-arch-build.ext\"\n");
		chdir(init_path);
		return -1;
	}

    if (sqlite3_open(dbname, &Database)) {
        fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
		chdir(init_path);
        return -1;
    }

	if (ExistsPkg(Data.name)) {
		fprintf(stderr, "Package %s already installed\n", PackageOrig);
		chdir(init_path);
		return 1;
	}

	if (GiveMeHash(pkgfullpathname, Data.crc) == -1) {
		chdir(init_path);
		return -1;
	}

	if (ExtractPackage(pkgfullpathname, &Data) == -1) {
		chdir(init_path);
		return -1;
	}
	InsertPkgDB(&Data);
	sqlite3_close(Database);
	chdir(init_path);
	fprintf(stdout, "Package %s installed\n", PackageOrig);
	return 0;
}

/**
 * This function downloads a package from the internet looking for its mirrors calling the SearchLinkForPackage 
 * @param name A package name
 * @return If no error ocurr, 0 is returned. If the package doesn't exists in any mirror, 1 is returned. In case of error, -1 (And an error message is issued)
 */
int DownloadPkg(char *name, char *out)
{
	ListOfLinks Links;
	char filename[PATH_MAX], pkgcrc[32];
	int i = 0, j = 0, fd = 0;
	char *input = NULL;
	char *MIRROR = NULL;

	Links.index = 0;
	Links.links = NULL;
	Links.comments = NULL;
	Links.crcs = NULL;

	/* Search for the package's links */
	/* If there is no, 1 is returned, so we leave here */
	if ((MIRROR = getenv("MIRROR"))) {
		if (SearchLinkForPackage(name, &Links, MIRROR))
			return 1;
	}
	else {
		if (SearchLinkForPackage(name, &Links, NULL))
			return 1;
	}

	if (Links.index > 1) {
		/* Print out all the links with their associated numbers */
		while(i < Links.index) {
			fprintf(stdout, "[%d] %s\n", i, Links.links[i]);
			i++;
		}
		i = 0;

		/* Input to get the link */
		while(1) {
			/* EOF leaves */
			if ((input = readline("Which one do you want to download? ")) == NULL) {
				freeLinks(&Links);
				free(input);
				return 1;
			}
			/* q leaves leaves */
			else if (*input == 'q') {
				freeLinks(&Links);
				free(input);
				return 1;
			}
			/* input the "c" letter and its number to get the link description */
			else if (*input == 'c') {
				i = atoi(input+1);
				if (i < Links.index && i >= 0)
					fprintf(stdout, "Comments for [%d]:\n%s\n", i, Links.comments[i]);
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
				if (i < Links.index && i >= 0) {
					free(input);
					break;
				}
				fprintf(stdout, "Invalid link given\n");
			}
		}
	}

	/* Get the output filename */
	for (j = strlen(Links.links[i])-1;Links.links[i][j]!='/' && j >=0;j--) ;
	if (Links.links[i][j] == '/') j++;
	sprintf(filename, "%s/%s", PACKAGES_DIRECTORY, &(Links.links[i][j]));

	/* If the output already exists and the CRC is ok, then great, we already have it, so there's no need to download it again */
	if ((fd = open(filename, O_RDONLY)) >= 0) {
		close(fd);
		if (GiveMeHash(filename, pkgcrc) == -1) {
			freeLinks(&Links);
			return -1;
		}
		if (strcmp(pkgcrc, Links.crcs[i]))
			fprintf(stderr, "CRC error, Downloading the package again...\n");
		else {
			if (out)
				strcpy(out, filename);
			freeLinks(&Links);
			return 0;
		}
	}

	/* Download the link */
	if (Download(Links.links[i], filename) == -1) {
		freeLinks(&Links);
		return -1;
	}
	if (out)
		strcpy(out, filename);

	/* Get the link hash */
	if (GiveMeHash(filename, pkgcrc) == -1) {
		freeLinks(&Links);
		return -1;
	}

	/* Does the link hash match with the one given in the database? */
	if (strcmp(pkgcrc, Links.crcs[i])) {
		freeLinks(&Links);
		fprintf(stderr, "CRC error, try to download your package again\n");
		return -1;
	}

	/* Great, we're cool, so we leave */
	freeLinks(&Links);

	return 0;
}

/**
 * This function update the mirror's databases, if one is supplied only that's updated, otherwise all mirrors.
 * @param name A database name (if NULL, then all databases)
 * @return If no error ocurr, 0 is returned. If the database given doesn't exists, 1 is returned. In case of error, -1 (And an error message is issued)
 */
int UpdateMirrorDB(char *db)
{
	int i = 0, fd = 0;
	char dbpath[PATH_MAX];
	char dbLink[PATH_MAX];
	char dbDesc[PATH_MAX];
	char *dbNiceName;
	DIR *dip;
	struct dirent *dit;

	/* If only one database was issued */
	if (db) {
		/* Get the path */
		snprintf(dbpath, PATH_MAX, "%s/%s.db", MIRRORS_DIRECTORY, db);
		if ((fd = open(dbpath, O_RDONLY)) < 0) {
			fprintf(stdout, "Database %s doesn't exists\n", db);
			return 1;
		}
		close(fd);
		memset(dbLink, '\0', sizeof(dbLink));
		memset(dbDesc, '\0', sizeof(dbDesc));
		/* Get its data */
		if (GetDataFromMirrorDatabase(dbpath, "LINK", dbLink) == -1)
			return -1;
		if (GetDataFromMirrorDatabase(dbpath, "DESC", dbDesc) == -1)
			return -1;
		if (dbDesc[0] != '\0')
			fprintf(stdout, "Comments for [%s]:\n%s\n", db, dbDesc);
		/* Get the database! */
		if (dbLink[0] != '\0') {
			if (Download(dbLink, dbpath) == -1)
				return -1;
		}

		return 0;
	}

	/* Get the databases from the directories */
	if ((dip = opendir(MIRRORS_DIRECTORY)) == NULL) {
		fprintf(stderr, "Couldn't open the mirror's database directory %s (%s)\n", MIRRORS_DIRECTORY, strerror(errno));
		return -1;
	}

	while ((dit = readdir(dip)) != NULL) {
		if (!strcmp(dit->d_name, ".") || !strcmp(dit->d_name, ".."))
			continue;
		snprintf(dbpath, PATH_MAX, "%s/%s", MIRRORS_DIRECTORY, dit->d_name);
		memset(dbLink, '\0', sizeof(dbLink));
		memset(dbDesc, '\0', sizeof(dbDesc));
		/* Get its data */
		if (GetDataFromMirrorDatabase(dbpath, "LINK", dbLink) == -1)
			return -1;
		if (GetDataFromMirrorDatabase(dbpath, "DESC", dbDesc) == -1)
			return -1;
		if (dbDesc[0] != '\0') {
			dbNiceName = strdup(dit->d_name);
			for (i = strlen(dbNiceName)-1;dbNiceName[i]!='.' && i>=0;i--) ;
			if (i > 0) dbNiceName[i] = '\0';
			fprintf(stdout, "Comments for [%s]:\n%s\n", dbNiceName, dbDesc);
			free(dbNiceName);
		}
		/* Get the database! */
		if (Download(dbLink, dbpath) == -1)
			return -1;
	}
	closedir(dip);

	return 0;
}


/**
 * This function update the mirror's databases, if one is supplied only that's updated, otherwise all mirrors.
 * @param name A database name (if NULL, then all databases)
 * @return If no error ocurr, 0 is returned. If the database given doesn't exists, 1 is returned. In case of error, -1 (And an error message is issued)
 */
int UpgradePkg(char *package)
{
	PkgData Data;
	ListOfPackages Packages;
	char *MIRROR = NULL;
	int ret = 0, fd = 0, PrevIndex = 0;

	memset(&Data, '\0', sizeof(Data));

	if (package) {
		/* Upgrade a single package */
		if ((fd = open(package, O_RDONLY)) >= 0) {
			/* Ok, the package exists */
			close(fd);
			if (FillPkgDataFromPackage(&Data, basename(package))) {
				fprintf(stdout, "Wrong package name format. It should be \"name-version-arch-build.ext\"\n");
				return -1;
			}
			if (RemovePkg(Data.name, 1) == -1) {
				fprintf(stderr, "Failed to remove the old version of %s\n", Data.name);
				return -1;
			}
			if (InstallPkg(package) == -1) {
				fprintf(stderr, "Could not install the new version of %s\n", Data.name);
				return -1;
			}
			return 0;
		}
		else {
			/* Ok the package wasn't phisically given */
			if ((ret =RemovePkg(package, 1)) == -1) {
				fprintf(stderr, "Failed to remove the old version of %s\n", Data.name);
				return -1;
			}
			else if (ret == 1)
				fprintf(stdout, "The package %s isn't installed. Installing the new version anyways\n", package);
			if (InstallPkg(package) == -1) {
				fprintf(stderr, "Could not install the new version of %s\n", package);
			}
			return 0;
		}
	}
	else {
#if 0
		/* Oooook, let's upgrade the whole thing */
		if (GetListOfPackages(Packages) == -1)
			return -1;
		OrigIndex = Packages.index;
		for (i=0;i<OrigIndex;i++) {
			if ((MIRROR = NewVersionAvailable(Packages))) {
				if (DownloadPkg(Packages.packages[i], NULL) == -1)
					continue;
				if (RemovePkg(Packages.packages[i], 1) == -1) {
					fprintf(stderr, "Failed to remove the old version of %s\n", Data.name);
					return -1;
				}
				setenv("MIRROR", MIRROR, 1);
				if (InstallPkg(Packages.packages[i]) == -1) {
					fprintf(stderr, "Could not install the new version of %s\n", Data.name);
					return -1;
				}
				free(MIRROR);
			}
			return 0;
		}
#else
		fprintf(stderr, "Functionality not implemented\n");
#endif
	}
}


/**
 * This function issue the help message if a wrong parameter is passed
 * @param status An exit status
 */
void say_help(int status)
{
	fprintf(stdout,
"Usage: kpkg OPTION package[s]\n"
" update           update the mirror's database\n"
" install          install local package or from a mirror\n"
" remove           remove a package from the system\n"
" search           search for a package in the database\n"
" provides         search for files inside of installed packages\n"
" download         download a package from a mirror"
"\n"
"Kpkg 4.0a by David B. Cortarello (Nomius) <dcortarello@gmail.com>\n");
	exit(status);
}

int main(int argc, char *argv[])
{
    int i = 1;
	int ret = 0;
	char *renv = NULL;

	/* Basic environment variables */
	if (!(renv = getenv("KPKG_DB_HOME")))
		dbname = strdup("/var/packages/installed.db");
	else
		dbname = strdup(renv);

	if (!(renv = getenv("ROOT")))
		HOME_ROOT = strdup("/");
	else
		HOME_ROOT = strdup(renv);

	/* Commands parsing */
	if (argc == 1) {
		free(dbname);
		free(HOME_ROOT);
		say_help(1);
	}
	if (!strcmp(argv[1], "help")) {
		free(dbname);
		free(HOME_ROOT);
		say_help(0);
	}
	else if (!strcmp(argv[1], "install"))
		while (argv[++i] != NULL)
			ret |= ret!=0?ret:InstallPkg(argv[i]);
	else if (!strcmp(argv[1], "remove"))
		while (argv[++i] != NULL)
			ret |= ret!=0?ret:RemovePkg(argv[i], 0);
	else if (!strcmp(argv[1], "search"))
		while (argv[++i] != NULL)
			ret |= SearchPkg(argv[i]);
	else if (!strcmp(argv[1], "provides"))
		while (argv[++i] != NULL)
			ret |= SearchFileInPkgDB(argv[i]);
	else if (!strcmp(argv[1], "download"))
		while (argv[++i] != NULL)
			ret |= DownloadPkg(argv[i], NULL);
	else if (!strcmp(argv[1], "update"))
		if (argv[2] == NULL)
			UpdateMirrorDB(NULL);
		else
			while (argv[++i] != NULL)
				ret |= UpdateMirrorDB(argv[i]);
	else if (!strcmp(argv[1], "upgrade"))
		if (argv[2] == NULL)
			UpgradePkg(NULL);
		else
			while (argv[++i] != NULL)
				ret |= UpgradePkg(argv[i]);
	free(dbname);
	free(HOME_ROOT);

	return ret;
}

