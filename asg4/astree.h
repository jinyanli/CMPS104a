#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
using namespace std;
#include "auxlib.h"
#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>
#include "astree.h"
#include <stack>

enum { ATTR_void, ATTR_bool, ATTR_char, ATTR_int, ATTR_null,
       ATTR_string, ATTR_struct, ATTR_array, ATTR_function,
       ATTR_variable, ATTR_field, ATTR_typeid, ATTR_param,
       ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr,ATTR_prototype,
       ATTR_bitset_size,
};
using attr_bitset = bitset<ATTR_bitset_size>;

struct symbol;
using symbol_table = unordered_map<string*,symbol*>;
using symbol_entry = symbol_table::value_type;

struct symbol {
    string func_name;
    string func_type;
    string sym_name;
   attr_bitset attributes;
   symbol_table* fields;
   size_t filenr, linenr, offset;
   size_t blocknr;
   string *struct_type;
   string *other_type;
   vector<symbol*>* parameters;
};

struct symbol_func{
  string funcname;
  string functype;
  string para1;
  string para1type; 
  string para2;
  string para2type;
  string para3;
  string para3type;
};



struct astree {
   int symbol;               // token code
   size_t filenr;            // index into filename stack
   size_t linenr;            // line number from source code
   size_t offset;            // offset of token with current line
   const string* lexinfo;    // pointer to lexical information
   attr_bitset attributes;     
   size_t blocknr;           // block number
   vector<astree*> children; // children of this n-way node
};


astree* new_astree (int symbol, int filenr, int linenr, int offset,
                    const char* lexinfo);
astree* adopt1 (astree* root, astree* child);
astree* adopt2 (astree* root, astree* left, astree* right);
astree* adopt3 (astree* root, astree* left, astree* middle,
                astree* right);
astree* adopt1sym (astree* root, astree* child, int symbol);
astree* changeSymbol(astree* root, int symbol);
astree* stealchildren(astree* theft, astree* tree);
void dump_astree (FILE* outfile, astree* root);
void yyprint (FILE* outfile, unsigned short toknum, astree* yyvaluep);
void free_ast (astree* tree);
void free_ast2 (astree* tree1, astree* tree2);

//set attributes
void set_attributes(astree* n);
const char *get_attr_string(attr_bitset attributes,symbol* sym);
void ast_tavesal(FILE* outfilesym,astree* root);
void build_struct_sym(astree* root);
void set_struct_field_type(astree* node, symbol* sym,
                         const string* struct_type);
string bitset_to_str (attr_bitset attributes);
void traverse_ast(astree* node);
void traverse_block(astree* node);
void set_var_type(astree* node, symbol* sym, const string* struct_name);
symbol* new_symbol(astree* node);
symbol_table* new_symbol_table (symbol* sym, string* key);
void insert_entry (symbol_table* table, string* key,symbol* sym);
RCSH("$Id: astree.h,v 1.1 2015-05-10 01:49:19-07 - - $")
#endif
