STRIP=strip
STRIP_FLAGS_STATIC=--strip-debug
STRIP_FLAGS_DYNAMIC=--strip-unneeded
CC=cc
CFLAGS=-O2 -pedantic -Wall -Werror
CFLAGS_DEBUG=-gdwarf-2 -g3
SQLITE3=/usr/lib/libsqlite3.a
CURL=/usr/lib/libcurl.a
SSL=/usr/lib/libssl.a /usr/lib/libcrypto.a
BZIP2=/usr/lib/libbz2.a
LZMA=/usr/lib/liblzma.a
ZLIB=/usr/lib/libz.a
LIBARCHIVE=/usr/lib/libarchive.a
READLINE=/usr/lib/libreadline.a
NCURSES=/usr/lib/libncurses.a
ACL=/usr/lib/libacl.a
DYNAMIC_KPKG_LDFLAGS=-lsqlite3 -larchive -lpthread -llzma -lbz2 -lz -lm -lc -lcurl -lrt -lssl -lcrypto -lreadline -lncurses -lacl -ldl
STATIC_KPKG_LDFLAGS=$(SQLITE3) $(CURL) $(SSL) $(LIBARCHIVE) $(BZIP2) $(LZMA) $(ZLIB) $(READLINE) $(NCURSES) $(ACL) -lrt -ldl -lpthread

LDFLAGS_DBCREATER=-lsqlite3 -lz

all: support.o sqlite_callbacks.o sqlite_backend.o file_operation.o db_creater.o kpkg.o
	$(CC) $(CFLAGS) -o kpkg kpkg.o sqlite_backend.o sqlite_callbacks.o support.o file_operation.o $(STATIC_KPKG_LDFLAGS)
	$(STRIP) $(STRIP_FLAGS_STATIC) kpkg
	$(CC) -o db_creater db_creater.o $(LDFLAGS_DBCREATER)
	$(STRIP) $(STRIP_FLAGS_DYNAMIC) db_creater

kpkg_dynamic: support.o sqlite_callbacks.o sqlite_backend.o file_operation.o kpkg.o
	$(CC) $(CFLAGS) -o kpkg_dynamic kpkg.o sqlite_backend.o sqlite_callbacks.o support.o file_operation.o $(DYNAMIC_KPKG_LDFLAGS)
	$(STRIP) $(STRIP_FLAGS_DYNAMIC) kpkg_dynamic

file_operation.o: file_operation.c datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c file_operation.c -o file_operation.o

sqlite_backend.o: sqlite_backend.c datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c sqlite_backend.c -o sqlite_backend.o

sqlite_callbacks.o: sqlite_callbacks.c datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c sqlite_callbacks.c -o sqlite_callbacks.o

support.o: support.c datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c support.c -o support.o

kpkg.o: sqlite_backend.h datastructs.h sqlite_callbacks.h version.h
	$(CC) $(CFLAGS) -c kpkg.c -o kpkg.o

db_creater.o: db_creater.c
	$(CC) -c db_creater.c -o db_creater.o

version.h:
	svn up | grep "^At" | awk -F '[ .]' '{print "#define VERSION " $$3 }' > version.h

clean:
	rm -f file_operation.o sqlite_backend.o sqlite_callbacks.o support.o kpkg.o version.h kpkg kpkg_dynamic db_creater
