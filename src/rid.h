#ifndef RID_H
#define RID_H

typedef int PageNum;
typedef int SlotNum;

class RID
{
public:
    RID() : pageNum(-1), slotNum(-1);
    RID(PageNum p, SlotNum s) : pageNum(p), slotNum(s);
    ~RID();
    RC GetPageNum (PageNum &p) const
    {
        p = this->pageNum;
        return 0;
    }
    RC GetSlotNum (SlotNum &s) const
    {
        s = this->slotNum;
        return 0;
    }
private:
    PageNum pageNum;
    SlotNum slotNum;
};

#endif //RID_H