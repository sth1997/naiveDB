#include "ix_internal.h"
#include "ix.h"
#include "btreenode.h"

#include <cstdio>
using namespace std;
#define CHECK_NONZERO(x) { \
    if ((rc = x)) { \
        if (rc < 0) \
            return rc; \
        else \
            printf("WARNING: %d \n", rc); } \
}

BTreeNode::BTreeNode(AttrType type, int len, PF_PageHandle& handle, bool needInit):
    attrType(type), attrLength(len), changed(false)
{
    //maxKeyNum * (PageNum + attrLength + RID) + PageNum*2 + keyNum <= PF_PAGE_SIZE
    maxKeyNum = (PF_PAGE_SIZE - sizeof(PageNum) * 2 - sizeof(int)) / (sizeof(PageNum) + attrLength + sizeof(RID));
    char* data;
    RC rc;
    rc = handle.GetData(data);
    if (rc < 0)
    {
        printf("ERROR: %d \n", rc);
        return;
    }
    else
        printf("WARNING: %d \n", rc);
    rc = handle.GetPageNum(thisPageNum);
    if (rc < 0)
    {
        printf("ERROR: %d \n", rc);
        return;
    }
    else
        printf("WARNING: %d \n", rc);
    pages = (PageNum*) data;
    pre = &pages[maxKeyNum];
    next = &pre[1];
    keys = (char*) &next[1];
    rids = (RID*) (keys + attrLength * maxKeyNum);
    keyNum = (int*) &rids[maxKeyNum];
    assert(((char*)keyNum) + sizeof(int) <= data + PF_PAGE_SIZE);
    assert(((char*)keyNum) + sizeof(int) > data + PF_PAGE_SIZE - sizeof(PageNum) - attrLength - sizeof(RID));
    if (needInit)
    {
        setNum(0);
        setPre(-1);
        setNext(-1);
    }
}

//return 1 if rid1 > rid2
//return 0 if rid1 = rid2
//return -1 if rid1 < rid2
int BTreeNode::CMP(const RID& rid1, const RID& rid2) const
{
    if (rid1.pageNum != rid2.pageNum)
        if (rid1.pageNum > rid2.pageNum)
            return 1;
        else
            return -1;
    else
        if (rid1.slotNum < rid2.slotNum)
            return -1;
        else
            return (rid1.slotNum > rid2.slotNum);
}

//return 1 if key1 > key2
//return 0 if key1 = key2
//return -1 if key1 < key2
int BTreeNode::CMP(const char* key1, const char* key2) const
{
    switch (attrType)
    {
        case INT:
            {
                int k1 = *((int*) key1);
                int k2 = *((int*) key2);
                if (k1 < k2)
                    return -1;
                return (k1 > k2);
            }
            break;
        case FLOAT:
            {
                float k1 = *((float*) key1);
                float k2 = *((float*) key2);
                if (k1 < k2)
                    return -1;
                return (k1 > k2);
            }
            break;
        case STRING:
            for (int i = 0; i < attrLength; ++i)
            {
                if (key1[i] != key2[i])
                {
                    if (key1[i] > key2[i])
                        return 1;
                    else
                        return -1;
                }
            }
            return 0;
            break;
        default:
            assert(0);
    }
}

//rid as the second keyword
//return 1 if (key1, rid1) > (key2, rid2)
//return 0 if (key1, rid1) = (key2, rid2)
//return -1 if (key1, rid1) < (key2, rid2)
int BTreeNode::CMP(const char* key1, const char* key2, const RID& rid1, const RID& rid2) const
{
    int cmpKey = CMP(key1, key2);
    if (cmpKey)
        return cmpKey;
    else
        return CMP(rid1, rid2);
}

//find the first position with less equal key
//return -1 means the input key is less than any key in this node
int BTreeNode::findLEPos(const char* key, const RID& rid) const
{
    assert(getNum() > 0);
    for (int i = 0; i < getNum(); ++i)
    {
        if (CMP(key, getKey(i), rid, getRID(i)) < 0)
            return i - 1;
    }
    return getNum() - 1;
}

// if pos == -1, compare all the keys
// if pos != -1, compare key[pos] and key[pos + 1]
bool BTreeNode::isInOrder(int pos) const
{
    if (pos != -1)
    {
        if (pos == getNum() - 1)
            return true;
        return (CMP(getKey(pos), getKey(pos + 1), getRID(pos), getRID(pos + 1)) == -1);
    }
    else
    {
        for (int i = 0; i < getNum() - 2; ++i)
            if (!isInOrder(i))
                return false;
        return true;
    }
}

// if this node is full, return an WARNING
// if (key, rid) is already in this node, return an ERROR
// if the keys are not in order after insertion, return an ERROR
RC BTreeNode::insert(const char* key, const RID& rid, const PageNum& page)
{
    if (getNum() >= maxKeyNum)
        return IX_NODEISFULL;
    this->changed = true;
    setNum(getNum() + 1);
    int i;
    for (i = getNum() - 2; i >= 0; --i)
    {
        int cmp = CMP(key, getKey(i), rid, getRID(i));
        if (cmp == 1)
            break;
        if (cmp == 0)
            return IX_ALREADYINNODE;
        // cmp == -1
        setRID(i + 1, getRID(i));
        setKey(i + 1, getKey(i));
        setPage(i + 1, getPage(i));
    }
    ++i;
    setRID(i, rid);
    setKey(i, key);
    setPage(i, page);
    //i-1, i, i + 1 is in order
    if (!isInOrder(i) || (i > 0 && !isInOrder(i - 1)))
        return IX_NOTINORDER;
    return OK_RC;
}

// if this node is empty, return an WARNING
// if this key is not in this node, return an ERROR
// if pos = -1, find (key, rid) and remove it
// if pos != -1, just remove the key[pos]
RC BTreeNode::remove(const char* key, const RID& rid, int pos)
{
    if (getNum() == 0)
        return IX_NODEISEMPTY;
    this->changed = true;
    if (pos != -1)
    {
        if (pos >= getNum())
            return IX_REMOVEOUTOFRANGE;
    }
    else
    {
        pos = findLEPos(key, rid);
        if (pos == -1)
            return IX_NOTINNODE;
        int cmp = CMP(key, getKey(pos), rid, getRID(pos));
        if (cmp != 0)
            return IX_NOTINNODE;
    }
    for (int i = pos; i < getNum() - 1; ++i)
    {
        setRID(i, getRID(i + 1));
        setKey(i, getKey(i + 1));
        setPage(i, getPage(i + 1));
    }
    setNum(getNum() - 1);
    return OK_RC;
}

// put the larger half of keys into otherNode;
// only work for the leaf node
RC BTreeNode::split(BTreeNode& otherNode)
{
    if (getNum() < maxKeyNum)
        return IX_SPLITTOOEARLY;
    if (!otherNode.isNew())
        return IX_SPLITWITHNOTEMPTY;
    setChanged();
    otherNode.setChanged();
    int middle = maxKeyNum >> 1;
    RC rc;
    for (int i = middle; i < getNum(); ++i)
        CHECK_NONZERO(otherNode.insert(getKey(i), getRID(i), getPage(i)));
    int delNum = getNum() - middle;
    for (int i = 0; i < delNum; ++i)
        CHECK_NONZERO(remove(NULL, RID(-1, -1), middle));
    
    otherNode.setNext(getNext());
    setNext(otherNode.getPageNum());
    otherNode.setPre(getPageNum());

    if (!isInOrder() || !otherNode.isInOrder())
        return IX_NOTINORDER;
    return OK_RC;
}

//only work for the leaf node