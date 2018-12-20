//
// ql.h
//   Query Language Component Interface
//

// This file only gives the stub for the QL component

#ifndef QL_H
#define QL_H

#include <stdlib.h>
#include <string.h>
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "parser.h"
#include <map>
#include <set>

//
// QL_Manager: query language (DML)
//
class QL_Manager {
public:
    QL_Manager (SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
    ~QL_Manager();                       // Destructor

    RC Select  (int nSelAttrs,           // # attrs in select clause
        const RelAttr selAttrs[],        // attrs in select clause
        int   nRelations,                // # relations in from clause
        const char * const relations[],  // relations in from clause
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Insert  (const char *relName,     // relation to insert into
        int   nValues,                   // # values
        const Value values[]);           // values to insert

    RC Delete  (const char *relName,     // relation to delete from
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Update  (const char *relName,     // relation to update
        const RelAttr &updAttr,          // attribute to update
        const int bIsValue,              // 1 if RHS is a value, 0 if attribute
        const RelAttr &rhsRelAttr,       // attr on RHS to set LHS equal to
        const Value &rhsValue,           // or value to set attr equal to
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

private:
    template<typename T>
    bool matchValue(CompOp op, T lValue, T rValue);
    RC CheckAttr(RelAttr attr,
        std::map<std::string,std::set<std::string> > &attr2rels,
        std::map<std::string, int> &relsCnt);
    RM_Manager* rm_mgr;          // RM_Manager object
    IX_Manager* ix_mgr;          // IX_Manager object
    SM_Manager* sm_mgr;          // SM_Manager object
};

//
// Print-error function
//
void QL_PrintError(RC rc);

// Warnings
#define QL_DATABASE_DOES_NOT_EXIST (START_QL_WARN + 0) // Database does not exist
#define QL_DATABASE_CLOSED         (START_QL_WARN + 1) // Database is closed
#define QL_RELATION_NOT_FOUND      (START_QL_WARN + 2) // relations not found
#define QL_RELATION_DUPLICATED     (START_QL_WARN + 3) // relations duplicated
#define QL_ATTR_NOT_FOUND          (START_QL_WARN + 4) // attributes not found
#define QL_ATTR_AMBIGUOUS          (START_QL_WARN + 5) // attributes ambiguous
#define QL_SELECT_REL_NOT_IN_FROM  (START_QL_WARN + 6) // select relations not in from clause
#define QL_SELECT_REL_DIDNT_HAVE_THIS_ATTR (START_QL_WARN + 7) // select relations didn't have this attribute
#define QL_INCOMPATIBLE_TYPE       (START_QL_WARN + 8) // condition type incompatible
#define QL_EOF                     (START_QL_WARN + 9) // ql eof
#define QL_NOT_SUPPORT_MULTI_JOIN_NOW (START_QL_WARN + 10) // ql didn't support multi join
#define QL_LASTWARN                QL_NOT_SUPPORT_MULTI_JOIN_NOW

// Errors
#define QL_INVALID_DATABASE_NAME   (START_QL_ERR  - 0) // Invalid database file name
#define QL_SMERROR                 (START_QL_ERR  - 0) // Invalid database file name
#define QL_UNIX                    (START_QL_ERR  - 1) // Unix error
#define QL_LASTERROR               QL_UNIX

#endif
