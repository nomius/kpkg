all:
	gcc -c file_operation.c -o file_operation.o
	gcc -c sqlite_backend.c -o sqlite_backend.o
	gcc -c sqlite_callbacks.c -o sqlite_callbacks.o
	gcc -c support.c -o support.o
	gcc -c kpkg.c -o kpkg.o
	gcc -o kpkg kpkg.o sqlite_backend.o sqlite_callbacks.o support.o file_operation.o -static -lsqlite3 -larchive -lpthread -ldl -llzma -lbz2 -lz -lm -lc -lcurl -lrt -lssl -lcrypto -lidn -lreadline -lncurses

clean:
	rm -f file_operation.o sqlite_backend.o sqlite_callbacks.o support.o kpkg.o kpkg
