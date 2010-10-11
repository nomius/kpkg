CC=cc
OPTS=-O2
LINKS=-lsqlite3 -larchive -lpthread -ldl -llzma -lbz2 -lz -lm -lc -lcurl -lrt -lssl -lcrypto -lidn -lreadline -lncurses

all: kpkg.o support.o sqlite_callbacks.o sqlite_backend.o file_operation.o
	$(CC) $(OPTS) -o kpkg kpkg.o sqlite_backend.o sqlite_callbacks.o support.o file_operation.o -static $(LINKS)

file_operation.o: file_operation.c datastructs.h sqlite_callbacks.h
	$(CC) $(OPTS) -c file_operation.c -o file_operation.o

sqlite_backend.o: sqlite_backend.c datastructs.h sqlite_callbacks.h
	$(CC) $(OPTS) -c sqlite_backend.c -o sqlite_backend.o

sqlite_callbacks.o: sqlite_callbacks.c datastructs.h sqlite_callbacks.h
	$(CC) $(OPTS) -c sqlite_callbacks.c -o sqlite_callbacks.o

support.o: support.c datastructs.h sqlite_callbacks.h
	$(CC) $(OPTS) -c support.c -o support.o

kpkg.o: sqlite_backend.h datastructs.h sqlite_callbacks.h
	$(CC) $(OPTS) -c kpkg.c -o kpkg.o

clean:
	rm -f file_operation.o sqlite_backend.o sqlite_callbacks.o support.o kpkg.o kpkg
