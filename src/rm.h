#ifndef RM_H
#define RM_H

#include "naivedb.h"
#include "rid.h"

class RM_Record {
public:
    RM_Record  ();
    ~RM_Record ();
    RC GetData    (char *&pData) const;
    RC GetRid     (RID &rid) const;
private:
    char* data;
    int recordSize;
    RID rid;
};

#define RM_NORECORD           (START_RM_ERR - 0)  // no record in RM_Record

#endif