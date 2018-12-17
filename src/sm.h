#ifndef SM_H
#define SM_H

#include "naivedb.h"
#include "rm.h"
#include "ix.h"
#include "catalog.h"
#include "parser.h"
#include <vector>

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
    RC PrintDBs();
    RC PrintTables();
    RC PrintTable(const char* relName);
    RC FindAllAttrs(const char* relName, std::vector<DataAttrInfo>& attrs);
    RC CheckRel(const char* relName, bool& found);
    RC CheckAttr(const char* relName, const char* attrName, bool& found);
    RC CheckCond(const Condition& cond, bool& found);
    RC FindRelForAttr(RelAttr& attr, int numRel, const char* const possibleRels[]);
    RC IsAttrIndexed(const char* relName, const char* attrName, bool& indexed);
    RC DBOpened() { return DBOpen; }
    RC FindRel(const char* relName, DataRelInfo& rel, RID& rid, bool& found);
    RC FindAttr(const char* relName, const char* attrName, DataAttrInfo& attrinfo, RID& rid, bool& found);
    RC ValidName(const char* name) const;
    void PrintAttrType(AttrType AttrType);
private:
    RM_Manager& rmm;
    IX_Manager& ixm;
    RM_FileHandle attrcat; // catalog for attributes
    RM_FileHandle relcat; // catalog for relations
    bool DBOpen;
    //char cwd[255]; // current working directory;
};

void SM_PrintError(RC rc);

#define SM_NULLDBNAME         (START_SM_WARN + 0) // dbName == NULL
#define SM_DBALREADYOPEN      (START_SM_WARN + 1) // db already open
#define SM_NOSUCHDB           (START_SM_WARN + 2) // no such db (can't find such db by dbName)
#define SM_DBNOTOPEN          (START_SM_WARN + 3) // db not open
#define SM_DBEXISTS           (START_SM_WARN + 4) // db already exists
#define SM_MKDIRERROR         (START_SM_WARN + 5) // get error when calling mkdir
#define SM_NAMETOOLONG        (START_SM_WARN + 6) // the input name is too long
#define SM_REMOVEERROR        (START_SM_WARN + 7) // get error when calling "rm -r"
#define SM_BADTABLEPARA       (START_SM_WARN + 8) // bad table parameters
#define SM_NOSUCHTABLE        (START_SM_WARN + 9) // no such table
#define SM_NOSUCHATTR         (START_SM_WARN + 10) // no such attribute
#define SM_INDEXEXISTS        (START_SM_WARN + 11) // index already exists
#define SM_NOSUCHINDEX        (START_SM_WARN + 12) // no such index
#define SM_OPENDIRERROR       (START_SM_WARN + 13) // get error when opening dir
#define SM_TYPEMISMATCH       (START_SM_WARN + 14) // the types of two attr mismatch
#define SM_NOSUCHENTRY        (START_SM_WARN + 15) // no such entry
#define SM_MULTIPLERELSASATISFY (START_SM_WARN + 16) // multiple relations satisfy (more than one relations have the attr)

#define SM_GETCWDERROR        (START_SM_ERR - 0) // get error when calling getcwd
#define SM_CHDIRERROR         (START_SM_ERR - 1) // get error when calling chdir
#define SM_ATTRNUMINCORRECT   (START_SM_ERR - 2) // the number of attributes is incorrect
#define SM_INVALIDCOMPOP      (START_SM_ERR - 3) // invalid comp op

#endif // SM_H
