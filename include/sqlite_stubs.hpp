#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// SQLite types
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

// SQLite result codes
#define SQLITE_OK           0
#define SQLITE_ROW          100
#define SQLITE_DONE         101

// SQLite text handling constants
#define SQLITE_STATIC       ((void(*)(void *))0)
#define SQLITE_TRANSIENT    ((void(*)(void *))-1)

// SQLite function declarations
int sqlite3_open(const char *filename, sqlite3 **ppDb);
int sqlite3_close(sqlite3 *db);
int sqlite3_exec(sqlite3* db, const char* sql, int (*callback)(void*,int,char**,char**), void* data, char** errmsg);
int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
int sqlite3_finalize(sqlite3_stmt *pStmt);
int sqlite3_step(sqlite3_stmt *pStmt);
const unsigned char *sqlite3_column_text(sqlite3_stmt *pStmt, int iCol);
int sqlite3_column_int(sqlite3_stmt *pStmt, int iCol);
int sqlite3_bind_text(sqlite3_stmt *stmt, int index, const char *text, int n, void(*destructor)(void*));
int sqlite3_bind_int(sqlite3_stmt *stmt, int index, int value);
void sqlite3_free(void *ptr);

#ifdef __cplusplus
}
#endif

// For filesystem operations on Unix platforms
#ifdef _WIN32
// Windows definitions
#include <windows.h>
#include <io.h>
// Explicitly define file open modes for Windows
#ifndef O_RDWR
#define O_RDWR      0x0002  // Open for reading and writing
#endif
#define S_IRUSR     _S_IREAD
#define S_IWUSR     _S_IWRITE
#define S_IRGRP     0
#define S_IROTH     0
#define close       _close
#define fchmod(fd, mode) 0
#else
// Unix definitions
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif 