/* vim: set sw=4 sts=4 */

/* 
 * Author David Bruno Cortarello <Nomius>. Redistribute under the terms of the
 * BSD-lite license. Bugs, suggests, nor projects: dcortarello@gmail.com
 *
 * Program: kpkg
 * Version: svn
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
#include "sqlite_backend.h"
#include "kpkg.h"
#include "support.h"
#include "file_operation.h"
#include "version.h"

/**
 * This function removes the whole package structure from PACKAGES and FILESPKG tables
 * @param name A package name
 * @param silent Since this function may get called from UpgradePkg, this will make the function to not print messages like "package not found".
 * @return If no error ocurr, 0 is returned. If the package doesn't exists, 1 is returned (And a message is issued). In case of error, -1 (And an error message is issued)
 */
int RemovePkg(char *name, int silent)
{
	char *pathb = getcwd(malloc(PATH_MAX), PATH_MAX);
	int ret = 0;
	PkgData Data;

	/* Let's open the database */
	if (sqlite3_open(dbname, &Database)) {
		fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
		free(pathb);
		return -1;
	}

	/* Switch to HOME_ROOT */
	if (chdir(HOME_ROOT)) {
		fprintf(stderr, "Can't go to %s (%s)\n", HOME_ROOT, strerror(errno));
		free(pathb);
		return -1;
	}

	/* Check if it is already installed */
	memset(&Data, '\0', sizeof(Data));
	strncpy(Data.name, name, PKG_NAME);
	Data.name[strlen(name)>PKG_NAME?PKG_NAME:strlen(name)] = '\0';

	if (!ExistsPkg(&Data)) {
		if (!silent)
			fprintf(stderr, "Package %s does not exist\n", name);
		chdir(pathb);
		free(pathb);
		return 1;
	}

	/* Remove all its files */
	if (RemovePkgFiles(name) == -1) {
		chdir(pathb);
		free(pathb);
		return -1;
	}

	/* Removing this package from the database */
	ret = RemovePkgData(name);
	sqlite3_close(Database);

	/* Go back and clean up this mess */
	chdir(pathb);
	free(pathb);
	fprintf(stdout, "Package %s removed\n", name);
	return ret;
}

/**
 * This function install a database in the MIRRORS_DIRECTORY
 * @param dbpath A database path
 * @return If no error ocurr, 0 is returned. In case of error, -1 (And an error message is issued).
 */
int InstKpkgDB(char *dbpath)
{
	int fdi = 0, fdo = 0, i = 0;
	char tmp[PATH_MAX], tmpbuf[8129];
	char *origdb = NULL;

	/* Create the destination database */
	origdb = basename(dbpath);
	snprintf(tmp, PATH_MAX, "%s/%s", MIRRORS_DIRECTORY, origdb);

	/* Open the user database */
	if ((fdi = open(dbpath, O_RDONLY)) < 0) {
		if (errno == ENOENT) {
			if (Download(dbpath, tmp) == -1)
				return -1;
			else
				return 0;
		}
		else {
			fprintf(stderr, "Can't open %s (%s)\n", dbpath, strerror(errno));
			return -1;
		}
	}

	if ((fdo = open(tmp, O_WRONLY|O_TRUNC|O_CREAT, 0644)) < 0) {
		fprintf(stderr, "Can't open %s for writing (%s)\n", dbpath, strerror(errno));
		return -1;
	}

	/* Read and write */
	while ((i = read(fdi, tmpbuf, PATH_MAX)) > 0)
		write(fdo, tmpbuf, i);

	close(fdi);
	close(fdo);

	/* If there was an error, show it and remove the destination database */
	if (i < 0) {
		fprintf(stderr, "Can't read from %s (%s)\n", dbpath, strerror(errno));
		unlink(tmp);
		return -1;
	}
	return 0;
}

/**
 * This function installs a package by calling ExtractPackage to extract it and InsertPkgDB to insert the package in PACKAGES and FILESPKG tables
 * @param name A package name
 * @return If no error ocurr, 0 is returned. If the package is already installed, 1 is returned (And a message is issued). In case of error, -1 (And an error message is issued)
 */
int InstallPkg(char *package)
{
	char *ptr_name = NULL, *pkgfullpath = NULL, *init_path = NULL, *input = NULL;
	char pkgfullpathname[PATH_MAX];
	char output[PATH_MAX];
	char PackageOrig[PATH_MAX];
	int fd = 0;
	PkgData Data;

	/* Save locations and initialize the package structure */
	strncpy(PackageOrig, package, PATH_MAX);
	PackageOrig[strlen(package)>PATH_MAX?PATH_MAX:strlen(package)] = '\0';
	init_path = getcwd(malloc(PATH_MAX), PATH_MAX);
	memset(&Data, '\0', sizeof(PkgData));

	/* Does it fisically exist? */
	if ((fd = open(package, O_RDONLY)) < 0) {
		/* Ok, the package doesn't exists, let's go for a mirror */
		if (DownloadPkg(package, output) != 0) {
			free(init_path);
			return -1;
		}
		package = output;
	}
	else
		close(fd);

	/* Get the full pathname instead of its relative name */
	ptr_name = basename(package);
	pkgfullpath = dirname(package);
	chdir(pkgfullpath);
	pkgfullpath = getcwd(malloc(PATH_MAX), PATH_MAX);
	snprintf(pkgfullpathname, PATH_MAX, "%s/%s", pkgfullpath, ptr_name);
	free(pkgfullpath);

	/* Switch to HOME_ROOT */
	if (chdir(HOME_ROOT)) {
		fprintf(stderr, "Can't go to %s (%s)\n", HOME_ROOT, strerror(errno));
		free(init_path);
		return -1;
	}

	/* Fill the package structure according to the package name */
	if (FillPkgDataFromPackage(&Data, ptr_name)) {
		fprintf(stdout, "Wrong package name format. It should be \"" PACKAGE_NAME_FORMAT "\"\n");
		chdir(init_path);
		return -1;
	}

	/* Open the database */
	if (sqlite3_open(dbname, &Database)) {
		fprintf(stderr, "Failed to open database %s (%s)\n", dbname, sqlite3_errmsg(Database));
		chdir(init_path);
		free(init_path);
		return -1;
	}

	/* Check if it is already installed */
	if (ExistsPkg(&Data)) {
		fprintf(stderr, "Package %s already installed\n", PackageOrig);
		chdir(init_path);
		free(init_path);
		return 1;
	}

	/* Get the package hash so we can register it in the database */
	if (GiveMeHash(pkgfullpathname, Data.crc) == -1) {
		chdir(init_path);
		free(init_path);
		return -1;
	}

	/* Extract the package itself */
	if (ExtractPackage(pkgfullpathname, &Data) == -1) {
		chdir(init_path);
		free(init_path);
		return -1;
	}
	PostInstall();

	/* Register the package in the database */
	InsertPkgDB(&Data);

	/* Clean everything up */
	sqlite3_close(Database);
	chdir(init_path);
	if (!noreadme)
		input = readline("Press intro to continue");
	free(input);
	fprintf(stdout, "Package %s installed\n", PackageOrig);
	return 0;
}

/**
 * This function downloads a package from the internet looking for its mirrors calling the SearchLinkForPackage 
 * @param name A package name
 * @param out Since this function may get called by InstallPkg, the full path to the package downloaded will be saved here. If null is given, this will not change.
 * @return If no error ocurr, 0 is returned. If the package doesn't exists in any mirror, 1 is returned. In case of error, -1 (And an error message is issued)
 */
int DownloadPkg(char *name, char *out)
{
	ListOfLinks Links;
	char filename[PATH_MAX], pkgcrc[32];
	int i = 0, j = 0, fd = 0;
	char *input = NULL;
	char *MIRROR = NULL;

	/* Initialize everything up */
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
	else if (SearchLinkForPackage(name, &Links, NULL))
		return 1;

	/* This is really ugly, and unfortunally we have to handle it */
	if (Links.index > 1) {
		/* Print out all the links with their associated numbers */
		while (i < Links.index) {
			fprintf(stdout, "[%d] %s\n", i, Links.links[i]);
			i++;
		}
		i = 0;

		/* Input to get the link */
		while (1) {
			/* EOF leaves */
			if ((input = readline("Which one do you want to download? ")) == NULL) {
				freeLinks(&Links);
				return 1;
			}
			/* q leaves leaves */
			else if (*input == 'q') {
				freeLinks(&Links);
				free(input);
				return 1;
			}
			/* input the "c" letter and its number to get the link comments */
			else if (*input == 'c') {
				i = atoi(input + 1);
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
	for (j = strlen(Links.links[i]) - 1; Links.links[i][j] != '/' && j >= 0; j--) ;
	if (Links.links[i][j] == '/')
		j++;
	sprintf(filename, "%s/%s", PACKAGES_DIRECTORY, &(Links.links[i][j]));

	/* If the output already exist and the CRC is ok, then great, we already have it, so there's no need to download it again */
	if ((fd = open(filename, O_RDONLY)) >= 0) {
		close(fd);
		if (GiveMeHash(filename, pkgcrc) == -1) {
			freeLinks(&Links);
			return -1;
		}
		if (strcmp(pkgcrc, Links.crcs[i]))
			fprintf(stderr, "CRC error, Downloading the package again...\n");
		else {
			/* Great, it's already installed, so we leave */
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
		snprintf(dbpath, PATH_MAX, "%s/%s.kdb", MIRRORS_DIRECTORY, db);
		if ((fd = open(dbpath, O_RDONLY)) < 0) {
			fprintf(stdout, "Database %s doesn't exists\n", db);
			return 1;
		}
		close(fd);

		/* Get its data */
		memset(dbLink, '\0', sizeof(dbLink));
		memset(dbDesc, '\0', sizeof(dbDesc));
		if (GetDataFromMirrorDatabase(dbpath, "LINK", dbLink) == -1)
			return -1;
		if (GetDataFromMirrorDatabase(dbpath, "DESC", dbDesc) == -1)
			return -1;

		/* Show comments for this database if any */
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
		/* Skip directories . and .. */
		if (!strcmp(dit->d_name, ".") || !strcmp(dit->d_name, ".."))
			continue;

		/* Get the full path name for that database */
		snprintf(dbpath, PATH_MAX, "%s/%s", MIRRORS_DIRECTORY, dit->d_name);

		/* Get its data */
		memset(dbLink, '\0', sizeof(dbLink));
		memset(dbDesc, '\0', sizeof(dbDesc));
		if (GetDataFromMirrorDatabase(dbpath, "LINK", dbLink) == -1)
			return -1;
		if (GetDataFromMirrorDatabase(dbpath, "DESC", dbDesc) == -1)
			return -1;

		/* Show comments for this database if any */
		if (dbDesc[0] != '\0') {
			dbNiceName = strdup(dit->d_name);

			/* Remove the extension to display it nicely with its comment */
			for (i = strlen(dbNiceName) - 1; dbNiceName[i] != '.' && i >= 0; i--) ;
			if (i > 0)
				dbNiceName[i] = '\0';
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
	char MIRROR[NAME_MAX];
	int ret = 0, i = 0;

	memset(&Data, '\0', sizeof(Data));

	if (package) {
		/* Upgrade a single package */
		if ((i = open(package, O_RDONLY)) >= 0) {
			/* Ok, the package itself was given */
			close(i);
			/* Get the package data from its name */
			if (FillPkgDataFromPackage(&Data, basename(package))) {
				fprintf(stdout, "Wrong package name format. It should be \"" PACKAGE_NAME_FORMAT "\"\n");
				return -1;
			}
			/* Remove the package */
			if (RemovePkg(Data.name, 1) == -1) {
				fprintf(stderr, "Failed to remove the old version of %s\n", Data.name);
				return -1;
			}
			/* Install the package */
			if (InstallPkg(package) == -1) {
				fprintf(stderr, "Could not install the new version of %s\n", Data.name);
				return -1;
			}
			return 0;
		}
		else {
			/* Ok the package wasn't actually given, but a name */
			strncpy(Data.name, package, PKG_NAME);
			Data.name[strlen(package)>PKG_NAME?PKG_NAME:strlen(package)] = '\0';
			if (!ExistsPkg(&Data)) {
				fprintf(stdout, "Package %s isn't installed (perhaps you meant install?)\n", package);
				return 1;
			}
			/* Check if there's a new version available on the mirror */
			if ((ret = NewVersionAvailable(&Data, MIRROR))) {
				setenv("MIRROR", MIRROR, 1);

				/* Great, found a new version, download it */
				if (DownloadPkg(Data.name, NULL) == -1)
					return -1;

				/* Remove the old one */
				if ((ret = RemovePkg(Data.name, 1)) == -1) {
					fprintf(stderr, "Failed to remove the old version of %s\n", Data.name);
					return -1;
				}

				/* Install it */
				if (InstallPkg(Data.name) == -1) {
					fprintf(stderr, "Could not install the new version of %s\n", Data.name);
				}
			}
			return 0;
		}
	}
	else {
		/* Oooook, let's upgrade the whole thing */
		if (GetListOfPackages(&Packages) == -1)
			return -1;
		for (i = 0; i < Packages.index; i++) {
			/* Let's serialize the PkgData structure */
			strcpy(Data.name, Packages.packages[i]);
			strcpy(Data.version, Packages.versions[i]);
			strcpy(Data.build, Packages.builds[i]);

			/* Check for new versions in mirrors */
			if ((ret = NewVersionAvailable(&Data, MIRROR)) == 1) {
				printf("Upgrading: [%s] [%s] [%s]\n", Data.name, Data.version, Data.build);
				setenv("MIRROR", MIRROR, 1);
				if (DownloadPkg(Data.name, NULL) == -1)
					continue;
				if (RemovePkg(Data.name, 1) == -1) {
					fprintf(stderr, "Failed to remove the old version of %s\n", Data.name);
					return -1;
				}
				if (InstallPkg(Data.name) == -1) {
					fprintf(stderr, "Could not install the new version of %s\n", Data.name);
					return -1;
				}
			}
			else if (ret == -1) {
				fprintf(stderr, "Failed to upgrade %s\n", Data.name);
				return -1;
			}
		}
	}
	return 0;
}


/**
 * This function issue the help message if a wrong parameter is passed
 * @param status An exit status
 */
static void say_help(int status)
{
	fprintf(stdout, "Usage: kpkg OPTION package[s]\n"
			" update           update the mirror's database\n"
			" install          install local package or from a mirror\n"
			" remove           remove a package from the system\n"
			" search           search for a package in the database\n"
			" upgrade          upgrade a package or the whole system\n"
			" provides         search for files inside of installed packages\n"
			" download         download a package from a mirror\n"
			" instkdb          install a database in the mirror's path\n");
	fprintf(stdout, "Kpkg %d by David B. Cortarello (Nomius) <dcortarello@gmail.com>\n\n", VERSION);
	exit(status);
}

int main(int argc, char *argv[])
{
	int i = 1;
	int ret = 0;
	char *renv = NULL;

	/* Basic environment variables */
	if ((renv = getenv("KPKG_DB_HOME")))
		dbname = strdup(renv);
	else
		dbname = strdup(KPKG_DB_HOME_DEFAULT);

	if ((renv = getenv("ROOT"))) {
		HOME_ROOT = strdup(renv);
		MIRRORS_DIRECTORY = malloc(sizeof(char)*PATH_MAX);
		PACKAGES_DIRECTORY = malloc(sizeof(char)*PATH_MAX);
		snprintf(MIRRORS_DIRECTORY, PATH_MAX, "%s/%s", HOME_ROOT, MIRRORS_DIRECTORY_DEFAULT);
		snprintf(PACKAGES_DIRECTORY, PATH_MAX, "%s/%s", HOME_ROOT, PACKAGES_DIRECTORY_DEFAULT);
	}
	else {
		HOME_ROOT = strdup("/");
		MIRRORS_DIRECTORY = strdup(MIRRORS_DIRECTORY_DEFAULT);
		PACKAGES_DIRECTORY = strdup(PACKAGES_DIRECTORY_DEFAULT);
	}

	noreadme = 0;
	if ((renv = getenv("NOREADME")))
		if (*renv)
			noreadme = 1;

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
			ret |= ret != 0 ? ret : InstallPkg(argv[i]);
	else if (!strcmp(argv[1], "remove"))
		while (argv[++i] != NULL)
			ret |= ret != 0 ? ret : RemovePkg(argv[i], 0);
	else if (!strcmp(argv[1], "search"))
		while (argv[++i] != NULL)
			ret |= SearchPkg(argv[i]);
	else if (!strcmp(argv[1], "provides"))
		while (argv[++i] != NULL)
			ret |= SearchFileInPkgDB(argv[i]);
	else if (!strcmp(argv[1], "download"))
		while (argv[++i] != NULL)
			ret |= DownloadPkg(argv[i], NULL);
	else if (!strcmp(argv[1], "instkdb"))
		while (argv[++i] != NULL)
			ret |= InstKpkgDB(argv[i]);
	else if (!strcmp(argv[1], "update")) {
		if (argv[2] == NULL)
			UpdateMirrorDB(NULL);
		else
			while (argv[++i] != NULL)
				ret |= UpdateMirrorDB(argv[i]);
	}
	else if (!strcmp(argv[1], "upgrade")) {
		if (argv[2] == NULL)
			UpgradePkg(NULL);
		else
			while (argv[++i] != NULL)
				ret |= UpgradePkg(argv[i]);
	}
	free(dbname);
	free(HOME_ROOT);
	free(MIRRORS_DIRECTORY);
	free(PACKAGES_DIRECTORY);

	return ret;
}
