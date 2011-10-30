CC=cc
STRIP=strip
CFLAGS=-O2 -pedantic -Wall -Werror
STRIP_FLAGS=--strip-unneeded
KPKG_LDFLAGS=-static -lsqlite3 -larchive -lpthread -llzma -lbz2 -lz -lm -lc -lcurl -lrt -lssl -lcrypto -lreadline -lncurses -lacl -ldl -lpthread
#CFLAGS=-gdwarf-2 -g3
#KPKG_LDFLAGS=-lsqlite3 -larchive -lpthread -llzma -lbz2 -lz -lm -lc -lcurl -lrt -lssl -lcrypto -lreadline -lncurses -lacl -ldl
LDFLAGS_DBCREATER=-lsqlite3 -lz

all: kpkg.o support.o sqlite_callbacks.o sqlite_backend.o file_operation.o db_creater.o
	$(CC) $(CFLAGS) -o kpkg kpkg.o sqlite_backend.o sqlite_callbacks.o support.o file_operation.o $(KPKG_LDFLAGS)
	$(STRIP) $(STRIP_FLAGS) kpkg
	$(CC) -o db_creater db_creater.o $(LDFLAGS_DBCREATER)
	$(STRIP) $(STRIP_FLAGS) db_creater

file_operation.o: file_operation.c datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c file_operation.c -o file_operation.o

sqlite_backend.o: sqlite_backend.c datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c sqlite_backend.c -o sqlite_backend.o

sqlite_callbacks.o: sqlite_callbacks.c datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c sqlite_callbacks.c -o sqlite_callbacks.o

support.o: support.c datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c support.c -o support.o

kpkg.o: sqlite_backend.h datastructs.h sqlite_callbacks.h
	$(CC) $(CFLAGS) -c kpkg.c -o kpkg.o

db_creater.o: db_creater.c
	$(CC) -c db_creater.c -o db_creater.o

clean:
	rm -f file_operation.o sqlite_backend.o sqlite_callbacks.o support.o kpkg.o kpkg db_creater
