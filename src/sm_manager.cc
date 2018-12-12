#include "sm.h"
#include <unistd.h>
#include <stddef.h>

using namespace std;
#define CHECK_NONZERO(x) { \
    if ((rc = x)) { \
        if (rc < 0) \
            return rc; \
        else \
            printf("ix_ihdl WARNING: %d \n", rc); } \
}

#define PRINT_NONZERO(x) { \
    if ((rc = x)) { \
        if (rc < 0) \
            printf("ERROR: %d \n", rc); \
        else \
            printf("ix_ihdl print WARNING: %d \n", rc); } \
}

SM_Manager::SM_Manager(RM_Manager& rm_Manager, IX_Manager& ix_Manager)
    :rmm(rm_Manager), ixm(ix_Manager), DBOpen(false)
{
    //memset(cwd, 0, sizeof(cwd));
}

SM_Manager::~SM_Manager()
{
    //TODO: handle open db
}

RC SM_Manager::ValidName(const char* name)
{
    if (name == NULL)
        return SM_NULLDBNAME;
    if (strlen(name) > MAXNAME)
        return SM_NAMETOOLONG;
    return OK_RC;
}

RC SM_Manager::CreateDb(const char* dbName)
{
    RC rc;
    CHECK_NONZERO(ValidName(dbName));
    if (DBOpen)
        return SM_DBALREADYOPEN;

    if (chdir(dbName) == 0)
    {
        if (chdir("../"))
            return SM_CHDIRERROR;
        return SM_DBEXISTS;
    }

    char command[128];
    sprintf(command, "mkdir %s", dbName);
    if (system(command))
        return SM_MKDIRERROR;

    //get into the db's directory
    if (chdir(dbName))
        return SM_CHDIRERROR;

    //create and open catalog
    CHECK_NONZERO(rmm.CreateFile("relcat", sizeof(DataRelInfo)));
    CHECK_NONZERO(rmm.OpenFile("relcat", relcat));
    CHECK_NONZERO(rmm.CreateFile("attrcat", sizeof(DataAttrInfo)));
    CHECK_NONZERO(rmm.OpenFile("attrcat", attrcat));

    // insert "relcat" and "attrcat" into the relcat file as two relations
    DataRelInfo relcatRel;
    DataRelInfo attrcatRel;
    strcpy(relcatRel.relName, "relcat");
    strcpy(attrcatRel.relName, "attrcat");
    relcatRel.recordSize = sizeof(DataRelInfo);
    attrcatRel.recordSize = sizeof(DataAttrInfo);
    relcatRel.attrCount = DataRelInfo::memberNumber();
    attrcatRel.attrCount = DataAttrInfo::memberNumber();

    RID rid;
    CHECK_NONZERO(relcat.InsertRec((char*) &relcatRel, rid));
    CHECK_NONZERO(relcat.InsertRec((char*) &attrcatRel, rid));

    // insert the attributes of DataRelInfo into the attrcat file
    DataAttrInfo attr;
    strcpy(attr.relName, "relcat");
    strcpy(attr.attrName, "recordSize");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataRelInfo, recordSize);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "relcat");
    strcpy(attr.attrName, "attrCount");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataRelInfo, attrCount);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "relcat");
    strcpy(attr.attrName, "relName");
    attr.attrLength = MAXNAME + 1;
    attr.attrType = STRING;
    attr.indexNo = -1;
    attr.offset = offsetof(DataRelInfo, relName);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    // insert the attributes of DataAttrInfo into the attrcat file
    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "offset");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, offset);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "attrType");
    attr.attrLength = sizeof(AttrType);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, attrType);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "attrLength");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, attrLength);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "indexNo");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, indexNo);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "relName");
    attr.attrLength = MAXNAME + 1;
    attr.attrType = STRING;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, relName);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "relcat");
    strcpy(attr.attrName, "attrName");
    attr.attrLength = MAXNAME + 1;
    attr.attrType = STRING;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, attrName);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    CHECK_NONZERO(rmm.CloseFile(relcat));
    CHECK_NONZERO(rmm.CloseFile(attrcat));

    if(chdir("../"))
        return SM_CHDIRERROR;

    return OK_RC;
}

RC SM_Manager::DestroyDb(const char* dbName)
{
    RC rc;
    CHECK_NONZERO(ValidName(dbName));
    if (DBOpen)
        return SM_DBALREADYOPEN;

    if (chdir(dbName))
        return SM_NOSUCHDB;

    if(chdir("../"))
        return SM_CHDIRERROR;
    
    char command[128];
    sprintf(command, "rm -r %s", dbName);
    if (system(command))
        return SM_REMOVEERROR;

    return OK_RC;
}

RC SM_Manager::OpenDb(const char* dbName)
{
    RC rc;
    CHECK_NONZERO(ValidName(dbName));
    if (DBOpen)
        return SM_DBALREADYOPEN;
    
    /*if (getcwd(cwd, sizeof(cwd)) != cwd)
        return SM_GETCWDERROR;*/
    
    if (chdir(dbName))
        return SM_NOSUCHDB;

    //open catalog
    CHECK_NONZERO(rmm.OpenFile("relcat", relcat));
    CHECK_NONZERO(rmm.OpenFile("attrcat", attrcat));
    DBOpen = true;
    return OK_RC;
}

RC SM_Manager::CloseDb()
{
    RC rc;
    if (!DBOpen)
        return SM_DBNOTOPEN;
    CHECK_NONZERO(rmm.CloseFile(relcat));
    CHECK_NONZERO(rmm.CloseFile(attrcat));

    if (chdir("../"))
        return SM_CHDIRERROR;
    DBOpen = false;
    return OK_RC;
}

RC SM_Manager::CreateTable(const char* relName, int attrCount, AttrInfo* attributes)
{
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    // TOCO


    return OK_RC;
}

RC SM_Manager::CreateIndex(const char* relName, const char* attrName)
{
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    CHECK_NONZERO(ValidName(attrName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    // TODO


    return OK_RC;
}