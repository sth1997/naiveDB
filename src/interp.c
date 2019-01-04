/*
 * interp.c: interpreter for RQL
 *
 * Authors: Dallan Quass (quass@cs.stanford.edu)
 *          Jan Jannink
 * originally by: Mark McAuliffe, University of Wisconsin - Madison, 1991
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <map>
#include <vector>
#include "naivedb.h"
#include "parser_internal.h"
#include "y.tab.h"

#include "sm.h"
#include "ql.h"
#include "time.h"

extern SM_Manager *pSmm;
extern QL_Manager *pQlm;

#define E_OK                0
#define E_INCOMPATIBLE      -1
#define E_TOOMANY           -2
#define E_NOLENGTH          -3
#define E_INVINTSIZE        -4
#define E_INVREALSIZE       -5
#define E_INVFORMATSTRING   -6
#define E_INVSTRLEN         -7
#define E_DUPLICATEATTR     -8
#define E_TOOLONG           -9
#define E_STRINGTOOLONG     -10
#define E_NOSUCHATTR        -11

/*
 * file pointer to which error messages are printed
 */
#define ERRFP stderr

/*
 * local functions
 */
static int mk_attr_infos(NODE *list, int max, AttrInfo attrInfos[]);
static int parse_format_string(char *format_string, AttrType *type, int *len);
static int mk_rel_attrs(NODE *list, int max, RelAttr relAttrs[]);
static void mk_rel_attr(NODE *node, RelAttr &relAttr);
static int mk_relations(NODE *list, int max, char *relations[]);
static int mk_setitems(NODE *list, int max, char* columns[], Value values[]);
static int mk_conditions(NODE *list, int max, Condition conditions[]);
static int mk_values(NODE *list, int max, Value values[]);
static void mk_value(NODE *node, Value &value);
static void print_error(char *errmsg, RC errval);
static void echo_query(NODE *n);
static void print_attrtypes(NODE *n);
static void print_op(CompOp op);
static void print_relattr(NODE *n);
static void print_value(NODE *n);
static void print_condition(NODE *n);
static void print_relattrs(NODE *n);
static void print_relations(NODE *n);
static void print_conditions(NODE *n);
static void print_values(NODE *n);

/*
 * interp: interprets parse trees
 *
 */

namespace{

void PrintError(RC rc)
{
   if (abs(rc) <= END_PF_WARN)
      PF_PrintError(rc);
   else if (abs(rc) <= END_RM_WARN)
      RM_PrintError(rc);
   else if (abs(rc) <= END_IX_WARN)
      IX_PrintError(rc);
   else if (abs(rc) <= END_SM_WARN)
      SM_PrintError(rc);
   else if (abs(rc) <= END_QL_WARN)
      QL_PrintError(rc);
   else
      std::cerr << "Error code out of range: " << rc << "\n";
}
}

RC interp(NODE *n)
{
   RC errval = 0;         /* returned error value      */

   /* if input not coming from a terminal, then echo the query */
   if(!isatty(0))
      echo_query(n);
   clock_t start_time=clock();
   switch(n -> kind){

      case N_CREATETABLE:            /* for CreateTable() */
         {
            int nattrs;
            AttrInfo attrInfos[MAXATTRS];

            /* Make sure relation name isn't too long */
            if(strlen(n -> u.CREATETABLE.relname) > MAXNAME){
               print_error((char*)"create", E_TOOLONG);
               break;
            }

            /* Make a list of AttrInfos suitable for sending to Create */
            nattrs = mk_attr_infos(n -> u.CREATETABLE.attrlist, MAXATTRS, 
                  attrInfos);
            if(nattrs < 0){
               print_error((char*)"create", nattrs);
               break;
            }

            /* Make the call to create */
            errval = pSmm->CreateTable(n->u.CREATETABLE.relname, nattrs, 
                  attrInfos);
            break;
         }   

      case N_CREATEINDEX:            /* for CreateIndex() */

         errval = pSmm->CreateIndex(n->u.CREATEINDEX.relname,
               n->u.CREATEINDEX.attrname);
         break;

      case N_DROPINDEX:            /* for DropIndex() */

         errval = pSmm->DropIndex(n->u.DROPINDEX.relname,
               n->u.DROPINDEX.attrname);
         break;

      case N_DROPTABLE:            /* for DropTable() */

         errval = pSmm->DropTable(n->u.DROPTABLE.relname);
         break;

      case N_LOAD:            /* for Load() */

         /*errval = pSmm->Load(n->u.LOAD.relname,
               n->u.LOAD.filename);*/
         printf("NOT support LOAD");
         break;

      case N_SET:                    /* for Set() */

         /*errval = pSmm->Set(n->u.SET.paramName,
               n->u.SET.string);*/
         printf("NOT support SET");
         break;

      case N_HELP:            /* for Help() */

         /*if (n->u.HELP.relname)
            errval = pSmm->Help(n->u.HELP.relname);
         else
            errval = pSmm->Help();*/
         printf("NOT support HELP");
         break;

      case N_PRINT:            /* for Print() */

         /*errval = pSmm->Print(n->u.PRINT.relname);*/
         printf("NOT support PRINT");
         break;

      case N_QUERY:            /* for Query() */
         {
            int       nSelAttrs = 0;
            RelAttr  relAttrs[MAXATTRS];
            int       nRelations = 0;
            char      *relations[MAXATTRS];
            int       nConditions = 0;
            Condition conditions[MAXATTRS];

            /* Make a list of RelAttrs suitable for sending to Query */
            nSelAttrs = mk_rel_attrs(n->u.QUERY.relattrlist, MAXATTRS,
                  relAttrs);
            if(nSelAttrs < 0){
               print_error((char*)"select", nSelAttrs);
               break;
            }

            /* Make a list of relation names suitable for sending to Query */
            nRelations = mk_relations(n->u.QUERY.rellist, MAXATTRS, relations);
            if(nRelations < 0){
               print_error((char*)"select", nRelations);
               break;
            }

            /* Make a list of Conditions suitable for sending to Query */
            nConditions = mk_conditions(n->u.QUERY.conditionlist, MAXATTRS,
                  conditions);
            if(nConditions < 0){
               print_error((char*)"select", nConditions);
               break;
            }

            /* Make the call to Select */
            errval = pQlm->Select(nSelAttrs, relAttrs,
                  nRelations, relations,
                  nConditions, conditions);
            break;
         }   

      case N_INSERT:            /* for Insert() */
         {
            int nValues = 0;
            Value values[MAXATTRS];

            //TODO: insert all lists at once
            NODE *valueLists = n->u.INSERT.valuelists;
            for (; valueLists != NULL; valueLists = valueLists->u.LIST.next)
            {
               /* Make a list of Values suitable for sending to Insert */
               NODE *valueList = valueLists->u.LIST.curr;
               nValues = 0;
               nValues = mk_values(valueList, MAXATTRS, values);
               if(nValues < 0){
                  print_error((char*)"insert", nValues);
                  break;
               }

               /* Make the call to insert */
               errval = pQlm->Insert(n->u.INSERT.relname,
                     nValues, values);
               if (errval)
                  PrintError(errval);
            }
            break;
         }   

      case N_DELETE:            /* for Delete() */
         {
            int nConditions = 0;
            Condition conditions[MAXATTRS];

            /* Make a list of Conditions suitable for sending to delete */
            nConditions = mk_conditions(n->u.DELETE.conditionlist, MAXATTRS,
                  conditions);
            if(nConditions < 0){
               print_error((char*)"delete", nConditions);
               break;
            }

            /* Make the call to delete */
            errval = pQlm->Delete(n->u.DELETE.relname,
                  nConditions, conditions);
            break;
         }   

      case N_UPDATE:            /* for Update() */
         {
            int nColumns = 0;
            char* columns[MAXATTRS];
            Value rhsValues[MAXATTRS];

            int nConditions = 0;
            Condition conditions[MAXATTRS];


            nColumns = mk_setitems(n->u.UPDATE.setitemList, MAXATTRS, columns, rhsValues);
            if (nColumns < 0)
            {
               print_error((char*)"update", nConditions);
               break;
            }

            /* Make a list of Conditions suitable for sending to Update */
            nConditions = mk_conditions(n->u.UPDATE.conditionlist, MAXATTRS,
                  conditions);
            if(nConditions < 0){
               print_error((char*)"update", nConditions);
               break;
            }
            //for (int i = 0; i < nColumns; ++i)
            //   printf("%s %d\n", columns[i], rhsValues[i].type);
            /* Make the call to update */
            /*errval = pQlm->Update(n->u.UPDATE.relname, relAttr, bIsValue, 
                  rhsRelAttr, rhsValue, nConditions, conditions);*/
            errval = pQlm->Update(n->u.UPDATE.relname, nColumns, columns, rhsValues, nConditions, conditions);
            break;
         }   

      case N_CREATEDATABASE:
         {
            if(strlen(n -> u.CREATEDATABASE.dbname) > MAXNAME){
               print_error((char*)"create", E_TOOLONG);
               break;
            }
            errval = pSmm->CreateDb(n->u.CREATEDATABASE.dbname);
            break;
         }

      case N_DROPDATABASE:
         {
            if(strlen(n -> u.DROPDATABASE.dbname) > MAXNAME){
               print_error((char*)"drop", E_TOOLONG);
               break;
            }
            errval = pSmm->DestroyDb(n->u.DROPDATABASE.dbname);
            break;
         }
      
      case N_USE:
         {
            if(strlen(n -> u.USE.dbname) > MAXNAME){
               print_error((char*)"use", E_TOOLONG);
               break;
            }
            errval = pSmm->OpenDb(n->u.USE.dbname);
            break;
         }

      case N_SHOWDATABASES:
         {
            errval = pSmm->PrintDBs();
            break;
         }

      case N_SHOWTABLES:
         {
            errval = pSmm->PrintTables();
            break;
         }

      case N_DESC:
         {
            errval = pSmm->PrintTable(n->u.DESC.tableName);
            break;
         }

      default:   // should never get here
         break;
   }
   clock_t end_time=clock();
   std::cout<< "Running time is: "<<static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000<<"ms"<<std::endl;
   return (errval);
}

/*
 * mk_attr_infos: converts a list of attribute descriptors (attribute names,
 * types, and lengths) to an array of AttrInfo's so it can be sent to
 * Create.
 *
 * Returns:
 *    length of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_attr_infos(NODE *list, int max, AttrInfo attrInfos[])
{
   int i;
   //int len;
   //AttrType type;
   NODE *attr;
   //RC errval;
   NODE *typeNode;
   std::vector<char*> primaryKeyNames;
   std::map<std::string, int> attrNames;
   int attrNum = 0;

   /* for each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next) {

      /* if the list is too long, then error */
      if(i == max)
         return E_TOOMANY;

      attr = list -> u.LIST.curr;
      if (attr->kind == N_PRIMARYKEY)
      {
         NODE *columnList = attr->u.PRIMARYKEY.columnListNode;
         NODE *column;
         for (int j = 0; columnList != NULL; ++j, columnList = columnList->u.LIST.next)
         {
            column = columnList->u.LIST.curr;
            primaryKeyNames.push_back(column->u.COLUMN.columnName);
         }
         continue;
      }

      /* Make sure the attribute name isn't too long */
      if(strlen(attr -> u.ATTRTYPE.attrname) > MAXNAME)
         return E_TOOLONG;

      /* interpret the format string */
      /*errval = parse_format_string(attr -> u.ATTRTYPE.type, &type, &len);
      if(errval != E_OK)
         return errval;*/

      /* add it to the list */
      attrInfos[attrNum].attrName = attr -> u.ATTRTYPE.attrname;
      attrNames[std::string(attrInfos[attrNum].attrName)] = attrNum;
      typeNode = attr->u.ATTRTYPE.attrType;
      attrInfos[attrNum].attrType = typeNode->u.TYPE.attrType;
      attrInfos[attrNum].attrLength = typeNode->u.TYPE.attrLength;
      attrInfos[attrNum].couldBeNULL = attr -> u.ATTRTYPE.couldBeNULL;
      attrNum++;
   }
   for (auto pkName : primaryKeyNames)
   {
      if (attrNames.find(std::string(pkName)) != attrNames.end())
      {
         i = attrNames[std::string(pkName)];
         attrInfos[i].isPrimaryKey = 1;
      }
      else
         return E_NOSUCHATTR;
   }
   return attrNum;
}

/*
 * mk_rel_attrs: converts a list of relation-attributes (<relation,
 * attribute> pairs) into an array of RelAttrs
 *
 * Returns:
 *    the lengh of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_rel_attrs(NODE *list, int max, RelAttr relAttrs[])
{
   int i;

   /* For each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      /* If the list is too long then error */
      if(i == max)
         return E_TOOMANY;

      mk_rel_attr(list->u.LIST.curr, relAttrs[i]);
   }

   return i;
}

/*
 * mk_rel_attr: converts a single relation-attribute (<relation,
 * attribute> pair) into a RelAttr
 */
static void mk_rel_attr(NODE *node, RelAttr &relAttr)
{
   relAttr.relName = node->u.RELATTR.relname;
   relAttr.attrName = node->u.RELATTR.attrname;
}

/*
 * mk_relations: converts a list of relations into an array of relations
 *
 * Returns:
 *    the lengh of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_relations(NODE *list, int max, char *relations[])
{
   int i;
   NODE *current;

   /* for each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      /* If the list is too long then error */
      if(i == max)
         return E_TOOMANY;

      current = list -> u.LIST.curr;
      relations[i] = current->u.RELATION.relname;
   }

   return i;
}

static int mk_setitems(NODE *list, int max, char* columns[], Value values[])
{
   int i;
   NODE *current;
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      if(i == max)
         return E_TOOMANY;
      current = list -> u.LIST.curr;
      columns[i] = current -> u.SETITEM.columnName;
      mk_value(current -> u.SETITEM.valueNode, values[i]);
   }
   return i;
}

/*
 * mk_conditions: converts a list of conditions into an array of conditions
 *
 * Returns:
 *    the lengh of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_conditions(NODE *list, int max, Condition conditions[])
{
   int i;
   NODE *current;

   /* for each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      /* If the list is too long then error */
      if(i == max)
         return E_TOOMANY;

      current = list -> u.LIST.curr;
      conditions[i].lhsAttr.relName = 
         current->u.CONDITION.lhsRelattr->u.RELATTR.relname;
      conditions[i].lhsAttr.attrName = 
         current->u.CONDITION.lhsRelattr->u.RELATTR.attrname;
      if (current->u.CONDITION.isOrNotNULL == 1 || current->u.CONDITION.isOrNotNULL == -1)
      {
         conditions[i].op = NO_OP;
         conditions[i].bRhsIsAttr = FALSE;
         if (current->u.CONDITION.isOrNotNULL == 1)
         {
            conditions[i].isNULL = 1;
            conditions[i].isNotNULL = 0;
         }
         else
         {
            conditions[i].isNULL = 0;
            conditions[i].isNotNULL = 1;
         }
         //printf("%s %s %d %d\n", conditions[i].lhsAttr.relName, conditions[i].lhsAttr.attrName, conditions[i].isNULL, conditions[i].isNotNULL);
      }
      else
      {
         conditions[i].op = current->u.CONDITION.op;
         if (current->u.CONDITION.rhsRelattr) {
            conditions[i].bRhsIsAttr = TRUE;
            conditions[i].rhsAttr.relName = 
               current->u.CONDITION.rhsRelattr->u.RELATTR.relname;
            conditions[i].rhsAttr.attrName = 
               current->u.CONDITION.rhsRelattr->u.RELATTR.attrname;
         }
         else {
            conditions[i].bRhsIsAttr = FALSE;
            mk_value(current->u.CONDITION.rhsValue, conditions[i].rhsValue);
         }
         conditions[i].isNULL = 0;
         conditions[i].isNotNULL = 0;
      }
   }

   return i;
}

/*
 * mk_values: converts a list of values into an array of values
 *
 * Returns:
 *    the lengh of the list on success ( >= 0 )
 *    error code otherwise
 */
static int mk_values(NODE *list, int max, Value values[])
{
   int i;

   /* for each element of the list... */
   for(i = 0; list != NULL; ++i, list = list -> u.LIST.next){
      /* If the list is too long then error */
      if(i == max)
         return E_TOOMANY;

      mk_value(list->u.LIST.curr, values[i]);
   }

   return i;
}

/*
 * mk_values: converts a single value node into a Value
 */
static void mk_value(NODE *node, Value &value)
{
   value.type = node->u.VALUE.type;
   switch (value.type) {
      case INT:
         value.data = (void *)&node->u.VALUE.ival;
         break;
      case FLOAT:
         value.data = (void *)&node->u.VALUE.rval;
         break;
      case STRING:
         value.data = (void *)node->u.VALUE.sval;
         break;
      case NULLTYPE:
         value.data = NULL;
         break;
   }
}

/*
 * parse_format_string: deciphers a format string of the form: xl
 * where x is a type specification (one of `i' INTEGER, `r' REAL,
 * `s' STRING, or `c' STRING (character)) and l is a length (l is
 * optional for `i' and `r'), and stores the type in *type and the
 * length in *len.
 *
 * Returns
 *    E_OK on success
 *    error code otherwise
 */
static int parse_format_string(char *format_string, AttrType *type, int *len)
{
   int n;
   char c;

   /* extract the components of the format string */
   n = sscanf(format_string, "%c%d", &c, len);

   /* if no length given... */
   if(n == 1){

      switch(c){
         case 'i':
            *type = INT;
            *len = sizeof(int);
            break;
         case 'f':
         case 'r':
            *type = FLOAT;
            *len = sizeof(float);
            break;
         case 's':
         case 'c':
            return E_NOLENGTH;
         default:
            return E_INVFORMATSTRING;
      }
   }

   /* if both are given, make sure the length is valid */
   else if(n == 2){

      switch(c){
         case 'i':
            *type = INT;
            if(*len != sizeof(int))
               return E_INVINTSIZE;
            break;
         case 'f':
         case 'r':
            *type = FLOAT;
            if(*len != sizeof(float))
               return E_INVREALSIZE;
            break;
         case 's':
         case 'c':
            *type = STRING;
            if(*len < 1 || *len > MAXSTRINGLEN)
               return E_INVSTRLEN;
            break;
         default:
            return E_INVFORMATSTRING;
      }
   }

   /* otherwise it's not a valid format string */
   else
      return E_INVFORMATSTRING;

   return E_OK;
}

/*
 * print_error: prints an error message corresponding to errval
 */
static void print_error(char *errmsg, RC errval)
{
   if(errmsg != NULL)
      fprintf(stderr, "%s: ", errmsg);
   switch(errval){
      case E_OK:
         fprintf(ERRFP, "no error\n");
         break;
      case E_INCOMPATIBLE:
         fprintf(ERRFP, "attributes must be from selected relation(s)\n");
         break;
      case E_TOOMANY:
         fprintf(ERRFP, "too many elements\n");
         break;
      case E_NOLENGTH:
         fprintf(ERRFP, "length must be specified for STRING attribute\n");
         break;
      case E_INVINTSIZE:
         fprintf(ERRFP, "invalid size for INTEGER attribute (should be %d)\n",
               (int)sizeof(int));
         break;
      case E_INVREALSIZE:
         fprintf(ERRFP, "invalid size for REAL attribute (should be %d)\n",
               (int)sizeof(real));
         break;
      case E_INVFORMATSTRING:
         fprintf(ERRFP, "invalid format string\n");
         break;
      case E_INVSTRLEN:
         fprintf(ERRFP, "invalid length for string attribute\n");
         break;
      case E_DUPLICATEATTR:
         fprintf(ERRFP, "duplicated attribute name\n");
         break;
      case E_TOOLONG:
         fprintf(stderr, "relation name or attribute name too long\n");
         break;
      case E_STRINGTOOLONG:
         fprintf(stderr, "string attribute too long\n");
         break;
      case E_NOSUCHATTR:
         fprintf(stderr, "no attributes correspond to the primary key");
         break;
      default:
         fprintf(ERRFP, "unrecognized errval: %d\n", errval);
   }
}

static void echo_query(NODE *n)
{
   switch(n -> kind){
      case N_CREATETABLE:            /* for CreateTable() */
         printf("create table %s (", n -> u.CREATETABLE.relname);
         print_attrtypes(n -> u.CREATETABLE.attrlist);
         printf(")");
         printf(";\n");
         break;
      case N_CREATEINDEX:            /* for CreateIndex() */
         printf("create index %s(%s);\n", n -> u.CREATEINDEX.relname,
               n -> u.CREATEINDEX.attrname);
         break;
      case N_DROPINDEX:            /* for DropIndex() */
         printf("drop index %s(%s);\n", n -> u.DROPINDEX.relname,
               n -> u.DROPINDEX.attrname);
         break;
      case N_DROPTABLE:            /* for DropTable() */
         printf("drop table %s;\n", n -> u.DROPTABLE.relname);
         break;
      case N_LOAD:            /* for Load() */
         printf("load %s(\"%s\");\n",
               n -> u.LOAD.relname, n -> u.LOAD.filename);
         break;
      case N_HELP:            /* for Help() */
         printf("help");
         if(n -> u.HELP.relname != NULL)
            printf(" %s", n -> u.HELP.relname);
         printf(";\n");
         break;
      case N_PRINT:            /* for Print() */
         printf("print %s;\n", n -> u.PRINT.relname);
         break;
      case N_SET:                                 /* for Set() */
         printf("set %s = \"%s\";\n", n->u.SET.paramName, n->u.SET.string);
         break;
      case N_QUERY:            /* for Query() */
         printf("select ");
         print_relattrs(n -> u.QUERY.relattrlist);
         printf("\n from ");
         print_relations(n -> u.QUERY.rellist);
         printf("\n");
         if (n->u.QUERY.conditionlist) {
            printf("where ");
            print_conditions(n->u.QUERY.conditionlist);
         }
         printf(";\n");
         break;
      case N_INSERT:            /* for Insert() */
         printf("insert into %s values ( ",n->u.INSERT.relname);
         printf("sth unnotated this print message!!!!!!");
         //print_values(n -> u.INSERT.valuelist);
         printf(");\n");
         break;
      case N_DELETE:            /* for Delete() */
         printf("delete %s ",n->u.DELETE.relname);
         if (n->u.DELETE.conditionlist) {
            printf("where ");
            print_conditions(n->u.DELETE.conditionlist);
         }
         printf(";\n");
         break;
      case N_UPDATE:            /* for Update() */
         {
            printf("update %s set ",n->u.UPDATE.relname);
            printf("sth unnotated this print message!!!!!!");
            ////print_relattr(n->u.UPDATE.relattr);
            //printf(" = ");
            //struct node *rhs = n->u.UPDATE.relorvalue;
//
            ///* The RHS can be either a relation.attribute or a value */
            //if (rhs->u.RELATTR_OR_VALUE.relattr) {
            //   /* Print out the relation.attribute */
            //   print_relattr(rhs->u.RELATTR_OR_VALUE.relattr);
            //} else {
            //   /* Print out the value */
            //   print_value(rhs->u.RELATTR_OR_VALUE.value);
            //}
            //if (n->u.UPDATE.conditionlist) {
            //   printf("where ");
            //   print_conditions(n->u.UPDATE.conditionlist);
            //}
            //printf(";\n");
            break;
         }
      default:   // should never get here
         break;
   }
   fflush(stdout);
}

static void print_attrtypes(NODE *n)
{
   NODE *attr;
   NODE *typeNode;
   const char* c;

   for(; n != NULL; n = n -> u.LIST.next){
      attr = n -> u.LIST.curr;
      typeNode = attr -> u.ATTRTYPE.attrType;
      AttrType t = typeNode->u.TYPE.attrType;
      if (t == INT)
         c = "INT";
      else if (t == STRING)
         c = "STRING";
      else if (t == FLOAT)
         c = "FLOAT";
      printf("%s = %s(%d)", attr -> u.ATTRTYPE.attrname, c, typeNode->u.TYPE.attrLength);
      if(n -> u.LIST.next != NULL)
         printf(", ");
   }
}

static void print_op(CompOp op)
{
   switch(op){
      case EQ_OP:
         printf(" =");
         break;
      case NE_OP:
         printf(" <>");
         break;
      case LT_OP:
         printf(" <");
         break;
      case LE_OP:
         printf(" <=");
         break;
      case GT_OP:
         printf(" >");
         break;
      case GE_OP:
         printf(" >=");
         break;
      case NO_OP:
         printf(" NO_OP");
         break;
   }
}

static void print_relattr(NODE *n)
{
   printf(" ");
   if (n->u.RELATTR.relname)
      printf("%s.",n->u.RELATTR.relname);
   printf("%s",n->u.RELATTR.attrname);
}  

static void print_value(NODE *n)
{
   switch(n -> u.VALUE.type){
      case INT:
         printf(" %d", n -> u.VALUE.ival);
         break;
      case FLOAT:
         printf(" %f", n -> u.VALUE.rval);
         break;
      case STRING:
         printf(" \"%s\"", n -> u.VALUE.sval);
         break;
      case NULLTYPE:
         printf(" NULL");
         break;
   }
}

static void print_condition(NODE *n)
{
   print_relattr(n->u.CONDITION.lhsRelattr);
   print_op(n->u.CONDITION.op);
   if (n->u.CONDITION.rhsRelattr)
      print_relattr(n->u.CONDITION.rhsRelattr);
   else
      print_value(n->u.CONDITION.rhsValue);
}

static void print_relattrs(NODE *n)
{
   for(; n != NULL; n = n -> u.LIST.next){
      print_relattr(n->u.LIST.curr);
      if(n -> u.LIST.next != NULL)
         printf(",");
   }
}

static void print_relations(NODE *n)
{
   for(; n != NULL; n = n -> u.LIST.next){
      printf(" %s", n->u.LIST.curr->u.RELATION.relname);
      if(n -> u.LIST.next != NULL)
         printf(",");
   }
}

static void print_conditions(NODE *n)
{
   for(; n != NULL; n = n -> u.LIST.next){
      print_condition(n->u.LIST.curr);
      if(n -> u.LIST.next != NULL)
         printf(" and");
   }
}

static void print_values(NODE *n)
{
   for(; n != NULL; n = n -> u.LIST.next){
      print_value(n->u.LIST.curr);
      if(n -> u.LIST.next != NULL)
         printf(",");
   }
}

