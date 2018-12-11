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
    RC OpenDb(const char* dbName);
    RC CloseDb();
    RC CreateTable(const char* relName, int attrCount, AttrInfo* attributes);
    RC DropTable(const char* relName);
    RC CreateIndex(const char* relName, const char* attrName);
    RC DropIndex(const char* relName, const char* attrName);
    RC PrintTables();
    RC PrintTable(const char* relName);
private:
    RM_Manager& rmm;
    IX_Manager& ixm;
    RM_FileHandle attrcat; // catalog for attributes
    RM_FileHandle relcat; // catalog for relations
    bool BDOpen;
    char* cwd[255]; // current working directory;
};

#endif // SM_H