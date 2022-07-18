STRIP=strip
STRIP_FLAGS_STATIC=--strip-debug
STRIP_FLAGS_DYNAMIC=--strip-unneeded
CC=cc
CFLAGS=-O2 -pedantic -Wall -fcommon
#CFLAGS_DEBUG=-gdwarf-2 -g3
SQLITE3=/usr/lib/libsqlite3.a
CURL=/usr/lib/libcurl.a
SSL=/usr/lib/libssl.a /usr/lib/libcrypto.a
BZIP2=/usr/lib/libbz2.a
LZMA=/usr/lib/liblzma.a
ZLIB=/usr/lib/libz.a
LIBARCHIVE=/usr/lib/libarchive.a
ACL=/usr/lib/libacl.a
LZO=/usr/lib/liblzo2.a
OPENSSL=/usr/lib/libssl.a
ZSTD=/usr/lib/libzstd.a
NGHTTP2=/usr/lib/libnghttp2.a
EXPAT=/usr/lib/libexpat.a
DYNAMIC_KPKG_LDFLAGS=-lsqlite3 -larchive -lpthread -llzma -lbz2 -lz -lm -lc -lcurl -lssl -lcrypto -lssl -lacl -ldl -lnghttp2 -lzstd
STATIC_KPKG_LDFLAGS=$(SQLITE3) $(CURL) $(SSL) $(LIBARCHIVE) $(BZIP2) $(LZMA) $(ZLIB) $(READLINE) $(NCURSES) $(ACL) $(RTMP) $(LZO) $(OPENSSL) $(ZSTD) $(NGHTTP2) $(EXPAT) -ldl -lpthread -lm

LDFLAGS_DBCREATER=-lsqlite3 -lz

all: support.o sqlite_callbacks.o sqlite_backend.o file_operation.o kpkg.o
	$(CC) $(CFLAGS) -o kpkg kpkg.o sqlite_backend.o sqlite_callbacks.o support.o file_operation.o $(STATIC_KPKG_LDFLAGS)
	$(STRIP) $(STRIP_FLAGS_STATIC) kpkg

kpkg_dynamic: support.o sqlite_callbacks.o sqlite_backend.o file_operation.o kpkg.o
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o kpkg_dynamic kpkg.o sqlite_backend.o sqlite_callbacks.o support.o file_operation.o $(DYNAMIC_KPKG_LDFLAGS)
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

version.h:
	echo "#define VERSION `git shortlog | grep -E '^[ ]+\w+' | wc -l`" > version.h

clean:
	rm -f file_operation.o sqlite_backend.o sqlite_callbacks.o support.o kpkg.o version.h kpkg kpkg_dynamic
