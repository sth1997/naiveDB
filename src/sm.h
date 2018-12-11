#ifndef SM_H
#define SM_H

#include "naivedb.h"
#include "rm.h"
#include "ix.h"
#include "catalog.h"

struct AttrInfo // used by parser
{
    char* attrName;
    AttrType attrType;
    int attrLength;
};

class SM_Manager
{
public:
    SM_Manager(RM_Manager& rm_Manager, IX_Manager& ix_Manager);
    ~SM_Manager();
    RC CreateDb(const char* dbName);
    RC DestroyDb(const char* dbName);
    RC OpenDb(const char* dbName);
    RC CloseDb();
    RC CreateTable(const char* relName, int attrCount, AttrInfo* attributes);
    RC DropTable(const char* relName);
    RC CreateIndex(const char* relName, const char* attrName);
    RC DropIndex(const char* relName, const char* attrName);
    RC PrintTables();
    RC PrintTable(const char* relName);
private:
    RC ValidName(const char* name);
    RM_Manager& rmm;
    IX_Manager& ixm;
    RM_FileHandle attrcat; // catalog for attributes
    RM_FileHandle relcat; // catalog for relations
    bool DBOpen;
    //char cwd[255]; // current working directory;
};

#define SM_NULLDBNAME         (START_SM_WARN + 0) // dbName == NULL
#define SM_DBALREADYOPEN      (START_SM_WARN + 1) // db already open
#define SM_NOSUCHDB           (START_SM_WARN + 2) // no such db (can't find such db by dbName)
#define SM_DBNOTOPEN          (START_SM_WARN + 3) // db not open
#define SM_DBEXISTS           (START_SM_WARN + 4) // db already exists
#define SM_MKDIRERROR         (START_SM_WARN + 5) // get error when calling mkdir
#define SM_NAMETOOLONG        (START_SM_WARN + 6) // the input name is too long
#define SM_REMOVEERROR        (START_SM_WARN + 7) // get error when calling "rm -r"

#define SM_GETCWDERROR        (START_SM_ERR + 0) // get error when calling getcwd
#define SM_CHDIRERROR         (START_SM_ERR + 1) // get error when calling chdir

#endif // SM_H