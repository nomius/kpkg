kpkg
====

kpkg is the binary Kwort package manager. It allows you to install, remove and download and upgrade packages.

Written in C, it make use of sqlite3, libarchive and curl APIs in order to perform its tasks.

## Build

Building kpkg will also build the db_creator binary required to create mirrors databases (you only need this program if you will create a mirror). In order to build both just type make:

> make

## Common errors

kpkg was developed using subversion heading versions to keep version tracking, if you see an error as the following while building:

> error: 'VERSION' undeclared

Please, run the following before building:

> echo '#define VERSION 100' > version.h

Then you can try building again.
