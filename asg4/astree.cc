#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
#include "astree.h"
#include "stringset.h"
#include "lyutils.h"
#include <cstring> 

FILE *symoutput;
//string * func_name;
symbol_table struct_table;
symbol_table global_table;
symbol_table* local_table;
int print_block_count = 0;
int next_block = 1;
//stack <symbol_table*> symbol_stack;
stack <int> block_count_stack;
int global_block_count=0;
//int depth = 0;
vector<symbol_table> symbol_stack;
bool find_type_break_flag=false;

bool search_ident_insymbol(string* ident, symbol_table* ptable);
void ast_tavesal_rec(astree* root);
void find_operator_type (astree* node);

astree* new_astree (int symbol, int filenr, int linenr, int offset,
                    const char* lexinfo) {
   astree* tree = new astree();
   tree->symbol = symbol;
   tree->filenr = filenr;
   tree->linenr = linenr;
   tree->offset = offset;
   tree->lexinfo = intern_stringset (lexinfo);
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           tree, tree->filenr, tree->linenr, tree->offset,
           get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}


astree* adopt1 (astree* root, astree* child) {
   root->children.push_back (child);
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
           root, root->lexinfo->c_str(),
           child, child->lexinfo->c_str());
   return root;
}

astree* adopt2 (astree* root, astree* left, astree* right) {
   adopt1 (root, left);
   adopt1 (root, right);
   return root;
}
astree* adopt3 (astree* root, astree* left, astree* middle,
      astree* right){
   adopt1 (root, left);
   adopt1 (root, middle);
   adopt1 (root, right);
   return root;

}

astree* adopt1sym (astree* root, astree* child, int symbol) {
   root = adopt1 (root, child);
   root->symbol = symbol;
   return root;
}

astree* changeSymbol(astree *root, int symbol) {
   root->symbol = symbol;
   return root;
}

astree* stealchildren(astree* theft, astree* tree){
     //adopt1(theft,tree);
    for(unsigned int i=0;i<tree->children.size();i++){
      adopt1(theft,tree->children.at(i));
   }
   tree->children.clear();
  return theft;
}

static void dump_node (FILE* outfile, astree* node) {
   const char *tname=get_yytname (node->symbol);
   if(strstr(tname,"TOK_")==tname)tname+=4;
/////////
   fprintf (outfile, "%s \"%s\" %ld.%ld.%02ld %s",
            tname, (node->lexinfo)->c_str(),
            node->filenr, node->linenr,
        node->offset,bitset_to_str(node->attributes).c_str());
/////////
      /*fprintf (outfile, "%s \"%s\" %ld.%ld.%02ld",
            tname, (node->lexinfo)->c_str(),
            node->filenr, node->linenr,
        node->offset);
       */
           /*,get_attr_string(node->attributes)*/
}

static void dump_astree_rec (FILE* outfile, astree* root, int depth) {
   if (root == NULL) return;
   for(int i=0;i<depth;i++){
      fprintf (outfile, "|     ");
   }
   
   dump_node (outfile, root);
   fprintf (outfile, "\n");
   for (size_t child = 0; child < root->children.size(); ++child) {
      dump_astree_rec (outfile, root->children[child], depth + 1);
   }
}

void dump_astree (FILE* outfile, astree* root) {   
   dump_astree_rec (outfile, root, 0);
   fflush (NULL);
}


void yyprint (FILE* outfile, unsigned short toknum, astree* yyvaluep) {
   DEBUGF ('f', "toknum = %d, yyvaluep = %p\n", toknum, yyvaluep);
   if (is_defined_token (toknum)) {
      dump_node (outfile, yyvaluep);
   }else {
      fprintf (outfile, "%s(%d)\n", get_yytname (toknum), toknum);
   }
   fflush (NULL);
}


void free_ast (astree* root) {
   while (not root->children.empty()) {
      astree* child = root->children.back();
      root->children.pop_back();
      free_ast (child);
   }
   DEBUGF ('f', "free [%X]-> %d:%d.%d: %s: \"%s\")\n",
           (uintptr_t) root, root->filenr, root->linenr, root->offset,
           get_yytname (root->symbol), root->lexinfo->c_str());
   delete root;
}

void free_ast2 (astree* tree1, astree* tree2) {
   free_ast (tree1);
   free_ast (tree2);
}
//ASG4---------------------------------------------------------------

//partner_functions-------------------------------------------
symbol_entry new_symbol_entry (string* key,symbol* sym) {
   // Creation of new nodes depending on type of information
   //symbol* symbol = new_symbol (node);         //potential memory leak

   // Using type_id to be the 'key' to the symbol entry
   symbol_entry new_entry = make_pair(key, sym);
   return new_entry;
}

void insert_entry (symbol_table* table, string* key,symbol* sym) {
   // Constructs a new symbol entry and inserts it into the table.
   symbol_entry new_entry = new_symbol_entry (key,sym);
   table->insert(new_entry);
}

symbol_table* new_symbol_table (symbol* sym, string* key) {
   // Does something extra besides just making new table by adding
   // an entry into the table as well
   symbol_table* new_table = new symbol_table ();//potential memory leak
   insert_entry (new_table,key,sym);
   return new_table;
}
/*
void enter_block (astree* node, string* key) {
   symbol_table* new_table = new symbol_table ();//potential memory leak
   insert_entry (new_table, node, key);

   // Pushes the table onto both the local and global stack
   // global stack is never popped
   //perm_sym_stack.push_back (new_table);
   //block_count_stack.push_back (new_table);
   //global_table.push_back (new_table);
    symbol_stack.push_back (new_table);
   // Increments next_block count
   next_block++;

   /// Accessing file number information //Debug Example
   //symbol_table* test = block_count_stack[next_block - 1];
   //cout << "filenr for " << *key << ": " << 
   //(*test).at(key)->filenr << endl;
   
}
*/
//partner_function_end-------------------------------------------

/////// For printing attributes
void enter_scope(){
  print_block_count+=1;
  next_block+=1;
  //block_count=block_count+1;
  global_block_count=global_block_count+1;
  block_count_stack.push(global_block_count);
  //cout<<global_block_count<<endl;
  //sym->global_block_count=global_block_count;

}
void leave_scope(){
  print_block_count-=1;
  next_block-=1;
  //cout<<block_count_stack.top()<<endl;
  block_count_stack.pop();
  

  //sym->global_block_count=sym->global_block_count-1;
}
const string attr_string [ATTR_bitset_size] {
       "void", "bool", "char", "int",
       "null", "string", "struct",
       "array", "function", "variable",
       "field", "typeid", "param",
       "lval", "const", "vreg", "vaddr"
};

string bitset_to_str (attr_bitset attributes) {
   // Takes the node's attr_bitset value and matches it
   // to the attributes.
   int bit_index;
   string attribute_string = "";
   for (bit_index = 0; bit_index < ATTR_bitset_size; ++bit_index) {
      int bitcheck = attributes.test(bit_index);
      if (bitcheck == 1) {
         attribute_string.append(" ");
         attribute_string.append(attr_string[bit_index]);
      }
   }
   return attribute_string;
}
/////////////




symbol* new_symbol(astree* node){
   symbol* sym = new symbol();
   //sym->funcname="";
   //sym->functype="";
   //sy
   sym->fields = NULL;
   sym->struct_type =new string;
   sym->other_type = new string;
   sym->filenr = node->filenr;
   sym->linenr = node->linenr;
   sym->offset = node->offset;
   sym->blocknr =0;
   sym->parameters = NULL;
  return sym;
}



//function-----------------------------------------------------
void set_funct_param_type(astree* node, symbol* sym,
                         const string* struct_type){  
   sym->attributes.set(ATTR_param);
   sym->attributes.set(ATTR_variable);
   sym->attributes.set(ATTR_lval);
   node->children[0]->attributes.set(ATTR_param);
   node->children[0]->attributes.set(ATTR_variable);
   node->children[0]->attributes.set(ATTR_lval);
  switch(node->symbol){
    case TOK_CHAR:
    sym->attributes[ATTR_char]=1;
    node->attributes[ATTR_char]=1;
    node->children[0]->attributes[ATTR_char]=1;
    break;
    case TOK_STRING:
    sym->attributes[ATTR_string]=1;
    node->attributes[ATTR_string]=1;
    node->children[0]->attributes[ATTR_string]=1;
    break;
    case TOK_TYPEID:
    node->attributes[ATTR_typeid]=1; 
    node->children[0]->attributes[ATTR_typeid]=1;
    sym->attributes[ATTR_struct]=1;
    sym->struct_type->append(*struct_type);
    sym->other_type->append(*struct_type);
    break;
    case TOK_INT:
    node->children[0]->attributes[ATTR_int]=1;
    node->attributes[ATTR_int]=1;
    sym->attributes[ATTR_int]=1;
    break;
    case TOK_VOID:
    fprintf (stderr,"VOID type in function args");
    //exit (EXIT_FAILURE);
    break;
    case TOK_BOOL:
    node->children[0]->attributes[ATTR_bool]=1;
    node->attributes[ATTR_bool]=1;
    sym->attributes[ATTR_bool]=1;
    break;
    case TOK_ARRAY:
      
    break;
  }
}

void build_paramlist(astree* param_node,symbol* func_sym){
 if(param_node->children.size()!=0){
    //enter_scope();
   for(size_t i = 0; i < param_node->children.size(); i++){
    symbol* sym=new_symbol(param_node->children[i]);
    sym->blocknr=1;
    set_funct_param_type(param_node->children[i],sym,
    param_node->children[i]->lexinfo);
    string* key;
//for(size_t j = 0; 
//j < param_node->children[i]->children[j]->children.size(); j++){
//if(param_node->children[i]->children[j]->symbol==TOK_DECLID){      
      if(param_node->children[i]->symbol==TOK_ARRAY)
       key=(string *)param_node->children[i]->children[1]->lexinfo;
       else
       key=(string *)param_node->children[i]->children[0]->lexinfo;
      //}
    //}
    fprintf(symoutput, "   %s (%ld.%ld.%ld) {%ld} %s\n",
    key->c_str(),sym->filenr,sym->linenr,sym->offset,
    sym->blocknr, get_attr_string(sym->attributes,sym)
    );
     func_sym->parameters->push_back(sym);
    }
  }
  fprintf(symoutput,"\n");
}

void func_print(string *key,symbol *sym){
        fprintf(symoutput, "\n%s (%ld.%ld.%ld) {%ld} %s\n",
          key->c_str(),sym->filenr,sym->linenr,sym->offset,
          sym->blocknr, get_attr_string(sym->attributes,sym)
          );
}

void build_function_sym(astree* func_node){
  string *key = NULL;
 // func_name;
  symbol* sym=new_symbol(func_node);
  if(func_node->symbol==TOK_PROTOTYPE){
   sym->attributes.set(ATTR_prototype);
   astree* temp=func_node->children[0]->children[0];
   temp->attributes.set(ATTR_prototype);  

  }else{
   sym->attributes.set(ATTR_function);
   func_node->children[0]->children[0]->attributes.set(ATTR_function);  
  }
 func_node->children[0]->children[0]->attributes.set(ATTR_function);

  sym->fields = new symbol_table();
  sym->parameters = new vector<symbol*>;
  astree* temp;
  for(size_t i = 0; i < func_node->children.size(); i++){
    switch (func_node->children[i]->symbol){
     case TOK_TYPEID:
        sym->attributes[ATTR_struct]=1;
        func_node->children[i]->attributes.set(ATTR_typeid);
        sym->struct_type->append(*func_node->children[i]->lexinfo);
        key=(string *)func_node->children[0]->children[0]->lexinfo;
        func_print(key,sym);
        build_paramlist(func_node->children[1],sym);
     break;
     case TOK_INT:
        sym->attributes.set(ATTR_int);
        func_node->children[i]->attributes.set(ATTR_int);
        func_node->children[0]->children[0]->attributes.set(ATTR_int);  
        key=(string *)func_node->children[i]->children[0]->lexinfo;
        func_print(key,sym);
        build_paramlist(func_node->children[1],sym);
     break;
     case TOK_CHAR:
          sym->attributes.set(ATTR_char);
          func_node->children[i]->attributes.set(ATTR_char);
          func_node->children[0]->children[0]->attributes[ATTR_char]=1;
          key=(string *)func_node->children[i]->children[0]->lexinfo;
          func_print(key,sym);
          build_paramlist(func_node->children[1],sym);
     break;
     case TOK_STRING:
          sym->attributes.set(ATTR_string);
          func_node->children[i]->attributes.set(ATTR_string);
          temp=func_node->children[0]->children[0];//
          temp->attributes[ATTR_string]=1;//
          key=(string *)func_node->children[i]->children[0]->lexinfo;
          func_print(key,sym);
          build_paramlist(func_node->children[1],sym);
     break;
     case TOK_BOOL:
         sym->attributes.set(ATTR_bool);
         func_node->children[i]->attributes.set(ATTR_bool);
         func_node->children[0]->children[0]->attributes.set(ATTR_bool);
         key=(string *)func_node->children[i]->children[0]->lexinfo;
         func_print(key,sym);
         build_paramlist(func_node->children[1],sym);
     break;
     case TOK_VOID:
         func_node->children[i]->attributes.set(ATTR_void);
         func_node->children[0]->children[0]->attributes.set(ATTR_void);
         sym->attributes.set(ATTR_void);
         key=(string *)func_node->children[i]->children[0]->lexinfo;
         func_print(key,sym);
         build_paramlist(func_node->children[1],sym);
     break; 
     case TOK_ARRAY:
        if(func_node->children[i]->children[0]->symbol==TOK_VOID){
         fprintf(stdout,"Error:void array return type in function\n");
         break;
       }     
       func_node->children[0]->children[0]->attributes[ATTR_function]=0;
       func_node->children[0]->children[1]->attributes[ATTR_function]=1;
       switch (func_node->children[0]->children[0]->symbol){
          case TOK_STRING:
         func_node->children[0]->children[0]->attributes[ATTR_string]=1;
         func_node->children[0]->children[1]->attributes[ATTR_string]=1;
          break;
          case TOK_INT:
         func_node->children[0]->children[0]->attributes[ATTR_int]=1;
          func_node->children[0]->children[1]->attributes[ATTR_int]=1;
          break;
          case TOK_CHAR:
        func_node->children[0]->children[0]->attributes[ATTR_char]=1;
       func_node->children[0]->children[1]->attributes[ATTR_char]=1;
          break;
        }
           
       sym->attributes.set(ATTR_array);
       key=(string *)func_node->children[i]->children[1]->lexinfo;
       func_print(key,sym);
       build_paramlist(func_node->children[1],sym);
     break; 
     case TOK_BLOCK:  
       traverse_ast(func_node->children[i]);
      break;

    }
 }
 insert_entry (&global_table,key, sym);
 fprintf(symoutput, "\n");
}

//function_end-----------------------------------------------------

//struct---------------------------------------------------

void set_struct_field_type(astree* node, symbol* sym,
                         const string* struct_type){  
  sym->attributes[ATTR_field]=1;
  node->attributes.set(ATTR_field);
  switch(node->symbol){
    case TOK_CHAR:
    sym->attributes[ATTR_char]=1;
    break;
    case TOK_STRING:
    sym->attributes[ATTR_string]=1;
    break;
    case TOK_TYPEID:
    sym->attributes[ATTR_struct]=1;
    node->attributes.set(ATTR_typeid);
    //sym->struct_type->append(*struct_type);
    //sym->other_type->append(*(node->lexinfo));
    sym->other_type->append(*struct_type);
    break;
    case TOK_INT:
    node->attributes.set(ATTR_int);
    sym->attributes[ATTR_int]=1;
    break;
    //case TOK_VOID:
    //sym->attributes[ATTR_void]=1;
    //break;
    case TOK_BOOL:
    node->attributes.set(ATTR_bool);
    sym->attributes[ATTR_bool]=1;
    break;

  }
}


void print_struct(string *key, symbol* struct_sym){
  fprintf(symoutput, "%s (%ld.%ld.%ld) {%ld} %s\n",
  key->c_str(), struct_sym->filenr,
  struct_sym->linenr, struct_sym->offset,
  struct_sym->blocknr, get_attr_string(struct_sym->attributes,
  struct_sym));
  for (auto it = struct_sym->fields->begin();
    it != struct_sym->fields->end(); ++it ){
    fprintf(symoutput, "  %s (%ld.%ld.%ld) %s\n",
    it->first->c_str(),
    it->second->filenr,
    it->second->linenr,
    it->second->offset,
    get_attr_string(it->second->attributes,it->second));
   }

}


void build_struct_fields(astree* root, symbol_table& fields){
  for(size_t i = 1; i < root->children.size(); i++){
      for(size_t j = 0; j < root->children[i]->children.size(); j++){
        symbol* sym = new_symbol(root->children[i]->children[j]);
        string *key = (string *)root->children[i]->children[j]->lexinfo;

        sym->struct_type->append(*root->children[0]->lexinfo);

        set_struct_field_type(root->children[i], sym,
        root->children[i]->lexinfo);

        fields.insert({key, sym});
       }

  }
}

void build_struct_sym(astree* root){
  symbol* struct_sym = new symbol();
  struct_sym->struct_type = new string;
  struct_sym->other_type = new string;
  struct_sym->attributes.set(ATTR_struct);
  struct_sym->fields = new symbol_table();
  build_struct_fields(root, *struct_sym->fields);
  string* key;
      if(root->children[0]->symbol == TOK_TYPEID){
        key = (string *)root->children[0]->lexinfo;
        struct_sym->struct_type->append(*root->children[0]->lexinfo);
        struct_sym->filenr = root->children[0]->filenr;
        struct_sym->linenr = root->children[0]->linenr;
        struct_sym->offset = root->children[0]->offset;
        struct_sym->blocknr = 0;
      }
  if(key == NULL){
    printf("NULL KEY\n");
  }else{
    print_struct(key, struct_sym);
    insert_entry(&struct_table, key, struct_sym);
//    struct_table.insert({key, struct_sym});
    //global_table.insert({key, struct_sym});
  
  }
 fprintf(symoutput, "\n");
}
//struct_end-------------------------------------------------------
//vardecl-----------------------------------------------------
void oper_check(astree* node){
   for(size_t i = 0; i < node->children.size(); i++){
   string *key;
   //auto search;
   unordered_map<string*,symbol*>::const_iterator got;
      switch( node->children[i]->symbol){
      case '+':
        // fprintf(stdout,"+\n");
        key=(string *)node->children[i]->children[0]->lexinfo;
        //cout <<*key<<endl;
         got= global_table.find(key);//(string*)node->lexinfo
         if(got->second->attributes[ATTR_array]==1){
         fprintf(stdout,"Error:can't add array\n");
         }     
      break;
     case TOK_LT:
       key=(string *)node->children[i]->children[0]->lexinfo;
       got= global_table.find(key);
       if(got->second->attributes[ATTR_array]==1){
       fprintf(stdout,"Error:can't compare pointers\n");
        }     
       break;
      }
   }
}

void find_type (astree* node, symbol* sym, string* struct_name){
  string *key;
  unordered_map<string*,symbol*>::const_iterator got;
  sym->attributes.set(ATTR_variable);
  sym->attributes.set(ATTR_lval);
  switch(node->symbol){
    case TOK_TYPEID:
      key=(string *)node->lexinfo;
     
      if(!search_ident_insymbol(key,&struct_table)){
       //fprintf(stdout,"Error:undefine type: %s at %ld.%ld.%02ld\n"
        //,(*node->lexinfo).c_str(),node->filenr,node->linenr,
         // node->offset);
        find_type_break_flag=true;
       break;
      }
      sym->attributes.set(ATTR_struct);
      sym->struct_type->append(*struct_name);
      break;
    case TOK_INT:
      sym->attributes.set(ATTR_int);
      node->attributes.set(ATTR_int);
      break;
    case TOK_VOID:  
      sym->attributes.set(ATTR_void);
      fprintf(stdout,"Error:can't have void vars\n");
      node->attributes.set(ATTR_void);
      break;
    case TOK_BOOL:
      sym->attributes.set(ATTR_bool);
      node->attributes.set(ATTR_bool);
      break;
    case TOK_CHAR:
      node->attributes.set(ATTR_char);
      sym->attributes.set(ATTR_char);
      break;
    case TOK_STRING:
      node->attributes.set(ATTR_string);
      sym->attributes.set(ATTR_string);
      break;
    case TOK_INTCON:
     sym->attributes.set(ATTR_int);
     sym->attributes.set(ATTR_const);
     node->attributes.set(ATTR_int);
     node->attributes.set(ATTR_const);
     break;
    case TOK_CHARCON:
      sym->attributes.set(ATTR_char);
      sym->attributes.set(ATTR_const);
      node->attributes.set(ATTR_const);
      node->attributes.set(ATTR_char);
      break;
    case TOK_STRINGCON:
      sym->attributes.set(ATTR_string);
      sym->attributes.set(ATTR_const);
      node->attributes.set(ATTR_const);
      node->attributes.set(ATTR_string);
      break;
    case TOK_ARRAY:
      sym->attributes.set(ATTR_array);
     break;
    
  }
}

void build_vardecl_sym (astree* node){
  symbol* sym_vardecl;
  string* key;
  oper_check(node);
  for(size_t i = 0; i < (node->children[0])->children.size(); i++){
    // Using a loop to check all the children nodes because there
    // are many different variations of where TOK_DECLID could be at

    // Handles looking for the variable name id
    if(((node->children[0])->children[i])->symbol == TOK_DECLID){
      key = (string*)(((node->children[0])->children[i])->lexinfo);
      
      sym_vardecl= new_symbol(node->children[0]->children[i]);
      sym_vardecl->blocknr=global_block_count;
       if((node->children[0])->symbol == TOK_ARRAY){
          sym_vardecl->attributes.set(ATTR_array);
         find_type (node->children[0]->children[0], sym_vardecl,
             (string*)(node->children[0]->children[0]->lexinfo));
       }
       
        if(find_type_break_flag==true)
            return;
      
      if(print_block_count==0){
      sym_vardecl->blocknr=print_block_count;
      }
    }

    //cout << *key << endl;//debug doenst work
    //cout << bitset_to_str (sym_struct->attributes) << endl;//debug

 }
  
  find_type (node->children[0], sym_vardecl,
             (string*)((node->children[0])->lexinfo));
  
  insert_entry(&global_table,key,sym_vardecl);  

  // Printing symbol information
  for (int i = 0; i < print_block_count; i++){
    fprintf(symoutput, "   ");
  }
  fprintf (symoutput, "%s (%ld.%ld.%ld) {%ld} %s\n",
            key->c_str(), sym_vardecl->filenr, sym_vardecl->linenr,
            sym_vardecl->offset, sym_vardecl->blocknr,
            get_attr_string(sym_vardecl->attributes, sym_vardecl));
}

//vardec_end-------------------------------------

//while----------------------------------------
void find_while_operand_type (astree* node){
  //string *key;
  //auto search;
 unordered_map<string*,symbol*>::const_iterator got;
  switch(node->symbol){
    case TOK_TYPEID:
    fprintf (stderr,"wrong compare type in while");
    exit (EXIT_FAILURE);
      break;
    case TOK_IDENT:
  /*
    key=(string *)node->lexinfo;
    cout <<*key<<endl;
    got= global_table.find(key);//(string*)node->lexinfo
     cout << *(got->first) << " is " << got->second;
   if(search != global_table.end()) {
     std::cout << "Found " << 
      search->first << " " << search->second << '\n';
     }
    */ 
      node->attributes.set(ATTR_variable);
      node->attributes.set(ATTR_lval);
      break;
    case TOK_INT:
       node->attributes.set(ATTR_int);
      break;
    case TOK_VOID:
      node->attributes.set(ATTR_void);
      break;
    case TOK_BOOL:
      node->attributes.set(ATTR_bool);
      break;
    case TOK_CHAR:
      node->attributes.set(ATTR_char);
      break;
    case TOK_STRING:
      node->attributes.set(ATTR_string);
      break;
    case TOK_INTCON:;
     node->attributes.set(ATTR_int);
     node->attributes.set(ATTR_const);
     break;
    case TOK_CHARCON:
      node->attributes.set(ATTR_const);
      node->attributes.set(ATTR_char);
      break;
    case TOK_STRINGCON:
      node->attributes.set(ATTR_const);
      node->attributes.set(ATTR_string);
      break;
    
  }
}
void find_operator_type (astree* node){
  switch(node->symbol){
      case TOK_EQ:
        node->attributes[ATTR_bool]=1;
        node->attributes[ATTR_vreg]=1;
        break;
      case TOK_NE:
         node->attributes[ATTR_vreg]=1;
         node->attributes[ATTR_bool]=1;
        break;
      case TOK_LT:
        node->attributes[ATTR_vreg]=1;
        node->attributes[ATTR_bool]=1;
        break;
      case TOK_LE:
        node->attributes[ATTR_vreg]=1;
        node->attributes[ATTR_bool]=1;
        break;
      case TOK_GT:
        node->attributes[ATTR_vreg]=1;
        node->attributes[ATTR_bool]=1;
        break;
      case TOK_GE:
        node->attributes[ATTR_vreg]=1;
        node->attributes[ATTR_bool]=1;
        break;
       }
}

void  build_while_sym(astree* node){
    find_operator_type(node->children[0]);

    for(size_t i = 0; i < node->children[0]->children.size(); i++){
      find_while_operand_type(node->children[0]->children[i]);
    }

   for(size_t i = 0; i < node->children.size(); i++){
       //if(node->children[i]->symbol==TOK_BLOCK)
        //traverse_ast(node->children[i]); 
        //else
        traverse_ast(node->children[i]);        
    }
}


//while_end----------------------------------------
//if_else-----------------------------------
void  build_ifelse_sym(astree* node){
      find_operator_type(node->children[0]);
    for(size_t i = 0; i < node->children[0]->children.size(); i++){
      find_while_operand_type(node->children[0]->children[i]);
    }


   for(size_t i = 0; i < node->children.size(); i++){
       //if(node->children[i]->symbol==TOK_BLOCK)
        traverse_ast(node->children[i]); 
    }
}
//if_else_end-----------------------------------

//call
void find_call_arg_type (astree* node){
    string *key;
    unordered_map<string*,symbol*>::const_iterator got;

  switch(node->symbol){
      case TOK_IDENT:
       key=(string *)node->lexinfo;
      if(!search_ident_insymbol(key, &struct_table)&&
          !search_ident_insymbol(key, &global_table)){
          node->attributes[ATTR_variable]=1;
            node->attributes[ATTR_lval]=1;
         //fprintf(stdout,
         //"Error:undefine type in call(): %s at %ld.%ld.%02ld\n"
         // ,(*key).c_str(),node->filenr,node->linenr,node->offset);
             }

      break;
    case TOK_TYPEID:
      node->attributes.set(ATTR_typeid);
      break;
    case TOK_INT:
       node->attributes.set(ATTR_int);
      break;
    case TOK_VOID:
      node->attributes.set(ATTR_void);
      break;
    case TOK_BOOL:
      node->attributes.set(ATTR_bool);
      break;
    case TOK_CHAR:
      node->attributes.set(ATTR_char);
      break;
    case TOK_STRING:
      node->attributes.set(ATTR_string);
      break;
    case TOK_INTCON:;
     node->attributes.set(ATTR_int);
     node->attributes.set(ATTR_const);
     break;
    case TOK_CHARCON:
      node->attributes.set(ATTR_const);
      node->attributes.set(ATTR_char);
      break;
    case TOK_STRINGCON:
      node->attributes.set(ATTR_const);
      node->attributes.set(ATTR_string);
      break;
    case '=':
    case '+':
    ast_tavesal_rec(node);
      break;
    
  }
}
//call_end
//-----------------------------------
bool search_ident_insymbol(string* ident, symbol_table* ptable){
   bool result =false;
   auto search = (*ptable).find(ident);
   if(search != (*ptable).end())
      result=true;
   return result;
}
//--------------------
void traverse_op(astree* node){
    string *key;
    //string *key1;
    //auto search;
    unordered_map<string*,symbol*>::const_iterator got;
    unordered_map<string*,symbol*>::const_iterator got1;
 if (node == NULL) return;
   switch (node->symbol) {
       case '-':
       case '*':
       case '/':
       case '%':
       case '=':
       case TOK_INDEX:
       case '+':
         
         for(size_t i = 0; i < node->children.size(); i++){
            key=(string *)node->children[i]->lexinfo; 

              
           if(node->children[i]->symbol==TOK_STRINGCON){
             node->children[i]->attributes[ATTR_string]=1;
             node->children[i]->attributes[ATTR_const]=1;
            fprintf(stdout,"Error:string '%s' as '+' operand\n",
               (*key).c_str());       
            }

           if(node->children[i]->symbol==TOK_CHARCON){
             node->children[i]->attributes[ATTR_char]=1;
             node->children[i]->attributes[ATTR_const]=1;
            fprintf(stdout,"Error:string '%s' as '+' operand\n"
            , (*key).c_str());       
            }

           if(node->children[i]->symbol==TOK_INT||
             node->children[i]->symbol==TOK_INTCON){
               node->children[i]->attributes[ATTR_const]=1;
              node->children[i]->attributes[ATTR_int]=1;
            }

           if(node->children[i]->symbol==TOK_IDENT){
              node->children[i]->attributes[ATTR_variable]=1;
              node->children[i]->attributes[ATTR_lval]=1;
            }           
          /* 
           if(!search_ident_insymbol(key, &struct_table)
              &&!search_ident_insymbol(key, &global_table)
              &&node->children[i]->symbol==TOK_IDENT){
            
            
               
              node->children[i]->attributes[ATTR_variable]=1;
              node->children[i]->attributes[ATTR_lval]=1;
             fprintf(stdout
            ,"Error:undefine type : %s at %ld.%ld.%02ld\n"
             ,(*key).c_str(),node->children[i]->filenr,
              node->children[i]->linenr,
              node->children[i]->offset);
             
             }
           */
           if(search_ident_insymbol(key, &global_table)){
              got= global_table.find(key);
              if(got->second->attributes[ATTR_array]==1){
              //fprintf(stdout,"Error:array '%s' as '+' operand\n",
                 //(*key).c_str());
               }
            }

        }

       break;

   }
}

void ast_tavesal_rec(astree* root){
   
   for(auto child : root->children){
      ast_tavesal_rec(child);
   }
  traverse_op(root);
   
}

void handle_re(astree* node){
   for(size_t i = 0; i < node->children.size(); i++){
       //if(node->children[i]->symbol==TOK_BLOCK)
        traverse_ast(node->children[i]); 
    }
}

void traverse_ast(astree* node) {
      string *key;
      unordered_map<string*,symbol*>::const_iterator got;
   if (node == NULL) return;
   switch (node->symbol) {
       case '.':
         node->attributes[ATTR_vaddr];
         node->attributes[ATTR_lval];
         break;
      case TOK_INTCON:                    
         node->attributes[ATTR_const]=1;  
      case TOK_INT:
         node->attributes[ATTR_int]=1;
         if(node->children.size() >= 1)
         node->children[0]->attributes[ATTR_int]=1;
         break;
      case TOK_CHARCON:
         node->attributes[ATTR_const]=1;
      case TOK_CHAR:
         node->attributes[ATTR_char]=1;
         if(node->children.size() >= 1)
         node->children[0]->attributes[ATTR_char]=1;
          break;
      case TOK_STRINGCON:
         node->attributes[ATTR_const]=1;          
      case TOK_STRING:
         node->attributes[ATTR_string]=1; 
         if(node->children.size() >= 1)
         node->children[0]->attributes[ATTR_string]=1;
         break;
      case TOK_VOID:
         node->attributes[ATTR_void]=1;     break;
      case TOK_BOOL:
         node->attributes[ATTR_bool]=1;
         if(node->children.size() >= 1)
         node->children[0]->attributes[ATTR_bool]=1;
         break;
      case TOK_STRUCT:
         build_struct_sym(node);
         node->attributes[ATTR_struct]=1;
         break;
      case TOK_IFELSE:
      case TOK_IF:
        build_ifelse_sym(node);
        break;
      case TOK_WHILE:
        build_while_sym(node);
        break;
      case TOK_NULL:
         node->attributes[ATTR_null]=1;
         node->attributes[ATTR_const]=1;
         break;
      case TOK_FIELD:
         node->attributes[ATTR_field]=1;    break;
      case TOK_PROTOTYPE:
         key=(string *)node->children[0]->children[0]->lexinfo;
         //search=(*global_table).find(key);
         //cout <<*key<<endl;
        
         //got= global_table.find(key);
         if(search_ident_insymbol(key,&global_table)){
            fprintf(stdout,"Error:duplicate prototype\n");
          }
         //if(got->second->attributes[ATTR_prototype]==1){
          //fprintf(stdout,"Error:duplicate prototype");
          //}
         //cout << *(got->first) << " is " << got->second;
 
         node->attributes[ATTR_prototype]=1;
         build_function_sym(node);
         break;
      case TOK_FUNCTION:
         //fprintf(stdout,"TOK_FUNCTION");
         build_function_sym(node);
         node->attributes[ATTR_function]=1;
         break;
      case TOK_CALL:
        node->attributes[ATTR_void]=1;
        node->children[0]->attributes[ATTR_function]=1;
        node->children[0]->attributes[ATTR_void]=1;
        for(size_t i = 1; i < node->children.size(); i++){
             find_call_arg_type(node->children[i]);
            }
        break;
      case TOK_ARRAY:
         node->attributes[ATTR_array]=1; 
         break;
      case TOK_VARDECL:
          build_vardecl_sym(node);
          node->attributes[ATTR_variable]=1;
          node->attributes[ATTR_lval]=1;
         break;
      case TOK_BLOCK:
           //block_count=block_count+1;
           //blocknr+=1;
           enter_scope();
           symbol_stack.push_back(global_table);
           //local_table=new symbol_table();
           for(size_t i = 0; i < node->children.size(); i++){
              traverse_ast(node->children[i]); 
            }
           leave_scope();
           symbol_stack.pop_back();
           //block_count-=1;
         break;
      case TOK_TYPEID:
         node->attributes[ATTR_struct]=1;
         node->attributes[ATTR_typeid]=1;
         break;
      case TOK_PARAMLIST:
         node->attributes[ATTR_param]=1;
         break;
      case '=':
      case '-':
      case '*':
      case '/':
      case '+':
        node->attributes[ATTR_vreg]=1;
        ast_tavesal_rec(node);
        break;
      case TOK_RETURN:
        handle_re(node);
      break;
      default: break;
   }
}


const char *get_attr_string(attr_bitset attributes,symbol* sym) {
   string attr = "";

   if (attributes[ATTR_void]==1)     attr += "void ";
   if (attributes[ATTR_bool]==1)     attr += "bool ";
   if (attributes[ATTR_char]==1)     attr += "char ";
   if (attributes[ATTR_null]==1)     attr += "null ";
   if (attributes[ATTR_string]==1)   attr += "string ";
   if (attributes[ATTR_field]==1){   attr += "field ";
   if (sym!=NULL){

          attr +="{";
          attr +=*(sym->struct_type);
          attr +="} ";
       }

   }
   if (attributes[ATTR_struct]==1){
       if(sym!=NULL){        
         if(attributes[ATTR_field]==0){ 

           attr += "struct ";
           attr +="\"";
           attr +=*(sym->struct_type);
           attr +="\" ";
          } 
         else{

           attr += "struct ";
           attr +="\"";
           attr +=*(sym->other_type);
           attr +="\" "; 
         }
       }
   }

   if (attributes[ATTR_int]==1)       attr += "int ";                 
   if (attributes[ATTR_array]==1)    attr += "array ";
   if (attributes[ATTR_function]==1) attr += "function ";
   if (attributes[ATTR_prototype]==1) attr += "prototype ";
   if (attributes[ATTR_variable]==1) attr += "variable ";
   if (attributes[ATTR_typeid]==1)   attr += "typeid ";
   if (attributes[ATTR_param]==1)    attr += "param ";
   if (attributes[ATTR_lval]==1)     attr += "lval ";
   if (attributes[ATTR_const]==1)    attr += "const ";
   if (attributes[ATTR_vreg]==1)     attr += "vreg ";
   if (attributes[ATTR_vaddr]==1)    attr += "vaddr ";
   return attr.c_str();
}


void ast_tavesal(FILE* outfilesym,astree* root){
  symoutput=outfilesym;
    for(size_t i = 0; i < root->children.size(); i++){
      traverse_ast(root->children[i]);
    }
      //ast_tavesal_rec(root);

}

RCSC("$Id: astree.cc,v 1.1 2015-05-10 01:49:19-07 - - $")

