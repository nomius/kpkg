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
 * This function extracts the package pointed by filename in the place where you are using libarchive
 * @param filename The tile to be extracted
 * @param Data The package data structure where all files extracted will be saved in order to be able to remove them
 * @return If no error ocurr, 0 is returned, otherwise -1 (And an error message is issued)
 */
int ExtractPackage(const char *filename, PkgData *Data)
{
	struct archive *a;
	struct archive_entry *entry;
	int flags = 0;

	/* Set extraction permissions to keep with those in the package */
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;
	flags |= ARCHIVE_EXTRACT_TIME;
	flags |=ARCHIVE_EXTRACT_UNLINK;

	/* Initialize the file extraction structure */
	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_compression_all(a);

	/* Open the file with libarchive */
	if (archive_read_open_file(a, filename, 10240) != ARCHIVE_OK) {
		fprintf(stderr, "Can't open file %s (%s)\n", filename, archive_error_string(a));
		return -1;
	}
	for (Data->files_num = 0, Data->files = NULL;archive_read_next_header(a, &entry) == ARCHIVE_OK; Data->files_num++) {
		/* Uncompress the file */
		if (archive_read_extract(a, entry, flags)) {
			fprintf(stderr, "Could not uncompress %s (%s)\n", archive_entry_pathname(entry), archive_error_string(a));
			continue;
		}

		/* Save the file to push it in the database */
		Data->files =  realloc(Data->files, (Data->files_num+1)*sizeof(char *));
		/* The next code is to remove the starting slash or dot slash */
		if (archive_entry_pathname(entry)[0] == '/')
			Data->files[Data->files_num] = strdup(archive_entry_pathname(entry)+1);
		else if (archive_entry_pathname(entry)[0] == '.' && archive_entry_pathname(entry)[1] == '/')
			Data->files[Data->files_num] = strdup(archive_entry_pathname(entry)+2);
		else
			Data->files[Data->files_num] = strdup(archive_entry_pathname(entry));
	}
	/* Clean up */
	archive_read_close(a);
	archive_read_finish(a);
	return 0;
}

/**
 * This function downloads a link and save it under the output filename using libcurl
 * @param link The link to be downloaded
 * @param output The filename where the downloaded link will be stored
 * @return If no error ocurr, 0 is returned, otherwise -1 (And an error message is issued)
 */
int Download(char *link, char *output)
{
	CURLcode res;
	CURL *curl = NULL;
	FILE *out_file = NULL;

	/* Open the destination output */
	if (!(out_file = fopen(output, "w"))) {
	    fprintf(stderr, "Can't create the output file %s (%s)\n", output, strerror(errno));
    	return -1;
	}

	/* Initialize curl */
	if (!(curl = curl_easy_init())) {
	    fprintf(stderr, "Couldn't initialize CURL\n");
		fclose(out_file);
	    return -1;
	}

	/* Set the curl options */
	fprintf(stdout, "Downloading [%s]...\n", link);
	curl_easy_setopt(curl, CURLOPT_URL, link);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, out_file);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

	/* Do the job */
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "An error ocurr. CURL returned %d\n", res);
		fclose(out_file);
		return -1;
	}
	fclose(out_file);
	return 0;
}

/**
 * This function gives you the crc32 of a file given using zlib
 * @param filename The filename that you want to get the crc32 for
 * @param hash The string where you want the crc32 to be saved
 * @return If no error ocurr, 0 is returned, otherwise -1 (And an error message is issued)
 */
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
		crc = crc32(crc, data, i);
	close(fd);

	sprintf(hash, "%lu", crc);

	return 0;
}
