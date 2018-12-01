#ifndef BTREENODE_H
#define BTREENODE_H
#include "naivedb.h"
#include "pf.h"
#include "rid.h"
#include "ix.h"
#include <assert.h>
#include <cstring>

//layout: PageNum * (maxKeyNum + 2), attrLength * maxKeyNum, RID * maxKeyNum, keyNum

class BTreeNode {
public:
    BTreeNode(AttrType type, int len, PF_PageHandle& handle, bool needInit = true);
    ~BTreeNode(){}
    RC insert(const char* key, const RID& rid, const PageNum& page);
    RC remove(const char* key, const RID& rid, int pos = -1);
    int getNum() const
    {
        return *keyNum;
    }
    bool nodeChanged() const
    {
        return changed;
    }
    int getMaxKeyNum() const
    {
        return maxKeyNum;
    }
    int findKeyGE(const char* key, const RID& rid) const;
    int findKey(const char* key, const RID& rid) const;
    RC split(BTreeNode& otherNode);
    void setChanged()
    {
        changed = true;
    }
    void setNext(const PageNum& _next)
    {
        *next = _next;
        changed = true;
    }
    PageNum getNext() const
    {
        return *next;
    }
    void setPre(const PageNum& _pre)
    {
        *pre = _pre;
        changed = true;
    }
    PageNum getPre() const
    {
        return *pre;
    }
    bool isNew() const
    {
        return (getNum() == 0 && getPre() == -1 && getNext() == -1);
    }
    PageNum getPageNum() const
    {
        return thisPageNum;
    }
    void copyKey(int pos, void* buf) const;
    RID getRID(int pos) const
    {
        assert(pos < *keyNum && pos >= 0);
        return rids[pos];
    }
    void setRID(int pos, const RID& rid)
    {
        assert(pos < *keyNum && pos >= 0);
        rids[pos] = rid;
        changed = true;
    }
    PageNum getPage(int pos) const
    {
        assert(pos < *keyNum && pos >= 0);
        return pages[pos];
    }
    void setPage(int pos, const PageNum& pn)
    {
        assert(pos < *keyNum && pos >= 0);
        pages[pos] = pn;
        changed = true;
    }
    char* getKey(int pos) const
    {
        assert(pos < *keyNum && pos >= 0);
        return keys + attrLength * pos;
    }
    void setKey(int pos, const char* key)
    {
        assert(pos < *keyNum && pos >= 0);
        memcpy(keys + attrLength * pos, key, attrLength);
        changed = true;
    }
    int CMP(const char* key1, const char* key2, const RID& rid1, const RID& rid2) const;
    int CMP(const char* key1, const char* key2) const;
    int CMP(const RID& rid1, const RID& rid2) const;
    char* getLargestKey() const
    {
        assert(getNum() > 0);
        return getKey(getNum() - 1);
    }
    RID getLargestRID() const
    {
        assert(getNum() > 0);
        return getRID(getNum() - 1);
    }

private:
    bool isInOrder(int pos = -1) const;
    void setNum(int num)
    {
        assert(num <= maxKeyNum && num >= 0);
        *keyNum = num;
        changed = true;
    }
    int* keyNum;
    int maxKeyNum;
    RID* rids;
    char* keys;
    PageNum* pages;
    PageNum* pre;
    PageNum* next;
    AttrType attrType;
    int attrLength;
    PageNum thisPageNum;
    bool changed;
};

#endif //BTREENODE_H