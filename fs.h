#ifndef FS_H
#define FS_H

int  fs_format();
void fs_debug();
int  fs_mount();

int  fs_create();
int  fs_delete( int inumber );
int  fs_getsize();

int  fs_read( int inumber, char *data, int length, int offset );
int  fs_write( int inumber, const char *data, int length, int offset );

#endif
