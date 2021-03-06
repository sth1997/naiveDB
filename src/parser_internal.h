/*
 * parser_internal.h: internal declarations for the REDBASE parser
 *
 * Authors: Dallan Quass
 *          Jan Jannink
 *
 * originally by: Mark McAuliffe, University of Wisconsin - Madison, 1991
 */

#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "parser.h"

/*
 * use double for real
 */
typedef float real;

/*
 * the prompt
 */
#define PROMPT	"\nREDBASE >> "

/*
 * REL_ATTR: describes a qualified attribute (relName.attrName)
 */
typedef struct{
    char *relName;      /* relation name        */
    char *attrName;     /* attribute name       */
} REL_ATTR;

/*
 * ATTR_VAL: <attribute, value> pair
 */
typedef struct{
    char *attrName;     /* attribute name       */
    AttrType valType;   /* type of value        */
    int valLength;      /* length if type = STRING */
    void *value;        /* value for attribute  */
} ATTR_VAL;

/*
 * all the available kinds of nodes
 */
typedef enum{
    N_CREATETABLE,
    N_CREATEINDEX,
    N_DROPTABLE,
    N_DROPINDEX,
    N_LOAD,
    N_SET,
    N_HELP,
    N_PRINT,
    N_QUERY,
    N_INSERT,
    N_DELETE,
    N_UPDATE,
    N_RELATTR,
    N_CONDITION,
    N_RELATTR_OR_VALUE,
    N_ATTRTYPE,
    N_VALUE,
    N_RELATION,
    N_STATISTICS,
    N_LIST,
    N_CREATEDATABASE,
    N_DROPDATABASE,
    N_USE,
    N_SHOWDATABASES,
    N_SHOWTABLES,
    N_TYPENODE,
    N_DESC,
    N_PRIMARYKEY,
    N_COLUMN,
    N_SETITEM
} NODEKIND;

/*
 * structure of parse tree nodes
 */
typedef struct node{
   NODEKIND kind;

   union{
      /* SM component nodes */
      /* create table node */
      struct{
         char *relname;
         struct node *attrlist;
      } CREATETABLE;

      /* create index node */
      struct{
         char *relname;
         char *attrname;
      } CREATEINDEX;

      /* drop index node */
      struct{
         char *relname;
         char *attrname;
      } DROPINDEX;

      /* drop table node */
      struct{
         char *relname;
      } DROPTABLE;

      /* load node */
      struct{
         char *relname;
         char *filename;
      } LOAD;

      /* set node */
      struct{
         char *paramName;
         char *string;
      } SET;

      /* help node */
      struct{
         char *relname;
      } HELP;

      /* print node */
      struct{
         char *relname;
      } PRINT;

      /* QL component nodes */
      /* query node */
      struct{
         struct node *relattrlist;
         struct node *rellist;
         struct node *conditionlist;
      } QUERY;

      /* insert node */
      struct{
         char *relname;
         struct node *valuelists;
      } INSERT;

      /* delete node */
      struct{
         char *relname;
         struct node *conditionlist;
      } DELETE;

      /* update node */
      struct{
         char *relname;
         struct node *setitemList;
         struct node *conditionlist;
      } UPDATE;

      /* command support nodes */
      /* relation attribute node */
      struct{
         char *relname;
         char *attrname;
      } RELATTR;

      /* condition node */
      struct{
         struct node *lhsRelattr;
         CompOp op;
         struct node *rhsRelattr;
         struct node *rhsValue;
         int isOrNotNULL; //1 means IS NULL, -1 means IS NOT NULL, 0 means it has right hand side value or attr
      } CONDITION;

      /* relation-attribute or value */
      struct{
         struct node *relattr;
         struct node *value;
      } RELATTR_OR_VALUE;

      /* <attribute, type> pair */
      struct{
         char *attrname;
         struct node *attrType;
         int couldBeNULL;
      } ATTRTYPE;

      /* <value, type> pair */
      struct{
         AttrType type;
         int  ival;
         real rval;
         char *sval;
      } VALUE;

      /* relation node */
      struct{
         char *relname;
      } RELATION;

      /* list node */
      struct{
         struct node *curr;
         struct node *next;
      } LIST;

      /* create database node */
      struct{
         char *dbname;
      } CREATEDATABASE;

      /* drop database node */
      struct{
         char *dbname;
      } DROPDATABASE;

      /* use node */
      struct{
         char *dbname;
      }  USE;

      /* type node */
      struct{
         AttrType attrType;
         int attrLength;
      } TYPE;

      /* desc node */
      struct{
         char *tableName;
      } DESC;

      /* primary key node */
      struct{
         struct node* columnListNode;
      } PRIMARYKEY;

      /* column node */
      struct
      {
         char *columnName;
      } COLUMN;

      /* setitem node */
      struct
      {
         char *columnName;
         struct node *valueNode;
      } SETITEM;
   } u;
} NODE;


/*
 * function prototypes
 */
NODE *newnode(NODEKIND kind);
NODE *create_table_node(char *relname, NODE *attrlist);
NODE *create_index_node(char *relname, char *attrname);
NODE *drop_index_node(char *relname, char *attrname);
NODE *drop_table_node(char *relname);
NODE *load_node(char *relname, char *filename);
NODE *set_node(char *paramName, char *string);
NODE *help_node(char *relname);
NODE *print_node(char *relname);
NODE *query_node(NODE *relattrlist, NODE *rellist, NODE *conditionlist);
NODE *insert_node(char *relname, NODE *valuelists);
NODE *delete_node(char *relname, NODE *conditionlist);
NODE *update_node(char *relname, NODE *setitemList,
		  NODE *conditionlist);
NODE *relattr_node(char *relname, char *attrname);
NODE *condition_node(NODE *lhsRelattr, CompOp op, NODE *rhsRelattrOrValue, int isOrNotNULL);
NODE *value_node(AttrType type, void *value);
NODE *relattr_or_value_node(NODE *relattr, NODE *value);
NODE *attrtype_node(char *attrname, NODE *type, int couldBeNULL);
NODE *relation_node(char *relname);
NODE *list_node(NODE *n);
NODE *prepend(NODE *n, NODE *list);
NODE *create_database_node(char *dbname);
NODE *drop_database_node(char *dbname);
NODE *use_node(char *dbname);
NODE *show_databases_node();
NODE *show_tables_node();
NODE *type_int_node(int len);
NODE *type_string_node(int len);
NODE *type_float_node();
NODE *desc_node(char *tableName);
NODE *primarykey_node(NODE *columnList);
NODE *column_node(char *columnName);
NODE *setitem_node(char *columnName, NODE *setitemList);


void reset_scanner(void);
void reset_charptr(void);
void new_query(void);
RC   interp(NODE *n);
int  yylex(void);
int  yyparse(void);

#endif




