#ifndef RID_H
#define RID_H

#include "naivedb.h"

typedef int PageNum;
typedef int SlotNum;

class RID
{
public:
    RID() : pageNum(-1), slotNum(-1){};
    RID(PageNum p, SlotNum s) : pageNum(p), slotNum(s){};
    ~RID() {};
    RC GetPageNum (PageNum &p) const
    {
        p = this->pageNum;
        return OK_RC;
    }
    RC GetSlotNum (SlotNum &s) const
    {
        s = this->slotNum;
        return OK_RC;
    }
    RC SetPageNum (PageNum p) {
        pageNum = p;
        return OK_RC;
    }
    RC SetSlotNum (SlotNum s) {
        slotNum = s;
        return OK_RC;
    }
    PageNum pageNum;
    SlotNum slotNum;
};

#endif //RID_H
