#include "rm.h"
#include <cstring>

RM_Record::RM_Record() : data(NULL), recordSize(-1)
{}

RM_Record::~RM_Record()
{
    if (data)
        delete []data;
}

RC RM_Record::GetData(char *&pData) const
{
    if (recordSize == -1)
        return RM_NORECORD;
    pData = new char[recordSize];
    memcpy(pData, data, recordSize);
    return OK_RC;
}

RC RM_Record::GetRid(RID &rid) const
{
    if (recordSize == -1)
        return RM_NORECORD;
    rid = this->rid;
    return OK_RC;
}