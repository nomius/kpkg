kpkg
====

kpkg is the binary Kwort package manager. It allows you to install, remove and download and upgrade packages.

Written in C, it make use of sqlite3, libarchive and curl APIs in order to perform its tasks.

## Build

Building kpkg will also build the db_creator binary required to create mirrors databases (you only need this program if you will create a mirror). In order to build both just type make:
```bash
make
```

## Install

Install procedure will only install kpkg in /usr/bin. To install the binary run make with the install parameter:
```bash
make install
```

### Common errors

kpkg was developed using subversion heading versions to keep version tracking, if you see an error as the following while building:
```
error: 'VERSION' undeclared
```
Please, run the following before building:
```bash
echo '#define VERSION 100' > version.h
```
Then you can try building again.

## Usage

To use kpkg, please, refer to the man file provided by typing:
```bash
man 8 kpkg
```

## Creating new databases

Kpkg uses by default the file /var/packages/installed.kdb as installed packages database. In case you want to create it from scratch you can do the following:

```bash
cat << EOF > init_kpkg.sql
CREATE TABLE PACKAGES(NAME TEXT KEY ASC, VERSION TEXT, ARCH TEXT, BUILD TEXT, EXTENSION TEXT, TEXT CRC, TEXT DATE);
CREATE TABLE FILESPKG(NAME TEXT, FILENAME TEXT NOT NULL, CRC TEXT);
EOF

sqlite3 -init init_kpkg.sql /var/packages/installed.kdb
```

