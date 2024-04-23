gcc -c -DBUILD_DLL sample.c
gcc -shared -o sample.dll sample.o -Wl
