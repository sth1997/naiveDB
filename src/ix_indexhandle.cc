#include "ix_internal.h"
#include "ix.h"

#include <iostream>
#include <cstring>
using namespace std;
#define CHECK_NONZERO(x) { \
    if ((rc = x)) { \
        if (rc < 0) \
            return rc; \
        else \
            printf("WARNING: %d \n", rc); } \
}

IX_IndexHandle::IX_IndexHandle(AttrType type, int len):
    attrType(type), attrLength(len), fileOpen(false)
    {}
