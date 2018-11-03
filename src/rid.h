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
    RC GetPageNum (PageNum &pageNum) const
    {
        return pageNum;
    }
    RC GetSlotNum (SlotNum &slotNum) const
    {
        return slotNum;
    }
private:
    PageNum pageNum;
    SlotNum slotNum;
};

#endif //RID_H