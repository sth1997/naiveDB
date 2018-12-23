#ifndef CATALOG_H
#define CATALOG_H

#include "naivedb.h"
#include "parser.h"
#include <cstring>

struct DataRelInfo
{
    DataRelInfo()
    {
        memset(relName, 0, MAXNAME + 1);
    }

    DataRelInfo(char* buf) // construct DataRelInfo from buf
    {
        memcpy(this, buf, sizeof(DataRelInfo));
    }

    DataRelInfo(const DataRelInfo& tmp)
    {
        memcpy(this, &tmp, sizeof(DataRelInfo));
    }

    DataRelInfo& operator=(const DataRelInfo& tmp)
    {
        if (this != &tmp)
            memcpy(this, &tmp, sizeof(DataRelInfo));
        return *this;
    }
    
    static int memberNumber()
    {
        return 3;
    }
    
    int recordSize;
    int attrCount;
    char relName[MAXNAME + 1];
};

struct DataAttrInfo
{
    DataAttrInfo()
    {
        offset = -1;
        indexNo = -1;
        memset(relName, 0, MAXNAME + 1);
        memset(attrName, 0, MAXNAME + 1);
        couldBeNULL = 0;
        isPrimaryKey = 0;
    }

    DataAttrInfo(char* buf) // construct DataRelInfo from buf
    {
        memcpy(this, buf, sizeof(DataAttrInfo));
    }

    DataAttrInfo(const AttrInfo& tmp, int input_offset, int input_indexNo, const char* input_relName)
    {
        offset = input_offset;
        attrType = tmp.attrType;
        attrLength = tmp.attrLength;
        indexNo = input_indexNo;
        strcpy(relName, input_relName);
        strcpy(attrName, tmp.attrName);
        couldBeNULL = tmp.couldBeNULL;
        isPrimaryKey = tmp.isPrimaryKey;
    }

    DataAttrInfo(const DataAttrInfo& tmp)
    {
        memcpy(this, &tmp, sizeof(DataAttrInfo));
    }

    DataAttrInfo& operator=(const DataAttrInfo& tmp)
    {
        if (this != &tmp)
            memcpy(this, &tmp, sizeof(DataAttrInfo));
        return *this;
    }

    static int memberNumber()
    {
        return 8;
    }

    int offset;
    AttrType attrType;
    int attrLength;
    int indexNo; // index number, -1 means no index
    char relName[MAXNAME + 1];
    char attrName[MAXNAME + 1];
    int couldBeNULL;
    int isPrimaryKey;
};

#endif //CATALOG_H