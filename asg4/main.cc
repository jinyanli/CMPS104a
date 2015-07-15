// $Id: main.cc,v 1.1 2015-05-10 01:49:19-07 - - $
//jinyan Li
//jli134@ucsc.edu
//asg2

#include <string>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include "auxlib.h"
#include "stringset.h"
#include <libgen.h>
using namespace std;
#include "astree.h"
#include "lyutils.h"
#include <vector>
#include <errno.h>
const string CPP = "/usr/bin/cpp";
const size_t LINESIZE = 1024;
string cppD="";
string yyin_cpp_command;
int numberOfOpt=0;
int Max=0;
int opt;
FILE *outstr, *outast, *outsym;


void scan_opts (int argc, char** argv) {
 yy_flex_debug=0;
 yydebug=0;

  while ((opt = getopt(argc, argv, "@:D:ly")) != -1) {
        
        switch (opt) {
        case '@':
            set_debugflags (optarg);
           numberOfOpt=numberOfOpt+2;
            break;
        case 'D':
              cppD=cppD+"-D" +optarg;  
                numberOfOpt=numberOfOpt+2;      
            break;
        case 'l':
            yy_flex_debug = 1;
               numberOfOpt++;

            break;
        case 'y':
             yydebug = 1;
           numberOfOpt++;

            break;
        default: 
            //errprintf("Error:wrong option\n");
            exit(EXIT_FAILURE); 
            break;
        }
      if(optind>Max)
       Max=optind;
      
    }
}


void yyin_cpp_popen (const char* filename) {
   //yyin_cpp_command = cpp_name;
   //yyin_cpp_command += " ";
   //yyin_cpp_command += filename;
   yyin_cpp_command= CPP + " "+cppD+" "+ filename;
   yyin = popen (yyin_cpp_command.c_str(), "r");
   if (yyin == NULL) {
      syserrprintf (yyin_cpp_command.c_str());
       exit(EXIT_FAILURE); 
     //exit (get_exitstatus());
   }
}

void yyin_cpp_pclose (void) {
   int pclose_rc = pclose (yyin);
   eprint_status (yyin_cpp_command.c_str(), pclose_rc);
   if (pclose_rc != 0) set_exitstatus (EXIT_FAILURE);
}

bool want_echo () {
   return not (isatty (fileno (stdin)) and isatty (fileno (stdout)));
}

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
   size_t len = strlen (string);
   if (len == 0) return;
   char* nlpos = string + len - 1;
   if (*nlpos == delim) *nlpos = '\0';
}


// Run cpp against the lines of the file.
void cpplines (FILE* pipe, char* filename) {
   
   int linenr = 1;
   char inputname[LINESIZE];
   strcpy (inputname, filename);
   for (;;) {
      char buffer[LINESIZE];
      char* fgets_rc = fgets (buffer, LINESIZE, pipe);
      
      if (fgets_rc == NULL) break;
      
      chomp (buffer, '\n');
      int sscanf_rc = sscanf (buffer, "# %d \"%[^\"]\"",
       &linenr, filename);
       if (sscanf_rc == 2) {
         continue;
      }
      char* savepos = NULL;
      char* bufptr = buffer;
      for (int tokenct = 1;; ++tokenct) {
         char* token = strtok_r (bufptr, " \t\n", &savepos);
         bufptr = NULL;
         if (token == NULL) break;
         
         intern_stringset (token);

      }
      ++linenr;
   }
   
}

string changeSuffix(string filename, string suffix){
   size_t foundDot= filename.find_last_of(".");
   filename= filename.substr(0,foundDot);
   filename= filename.append ("."+suffix);
return filename;
}



int main (int argc, char** argv) {  
 set_execname (argv[0]);
 

scan_opts (argc, argv);
///////////////////////////////
if(Max>(argc-1)){
 errprintf ("Usage: %s [-ly] [filename]\n", get_execname());
    //return 1;
       exit(EXIT_FAILURE);
}

if(argc==1) {
    errprintf("Error:no file is accepted\n");
    //return 1;
       exit(EXIT_FAILURE);
  }

 if((argc-numberOfOpt-1)>1) {
    errprintf("Error:Program only accepts one file\n");
    //return 1;
     exit(EXIT_FAILURE);
  }

   string findfile;
   size_t foundOc; 
   string checkOc;

   findfile=argv[argc-1];
   //printf("%s\n",findfile.c_str());
   foundOc=findfile.find_last_of(".");
    //printf("%d\n",foundOc);
   checkOc=findfile.substr(foundOc+1,string::npos);//
    //printf("%s\n", checkOc.c_str());

   if(checkOc!="oc") {
    errprintf("Error:Not .oc file\n");
    //return 1;
       exit(EXIT_FAILURE);
  }
////////////////////////////////////////////////// 

 
   char* filename = argv[argc - 1];
   //printf ("command=\"%s\"\n", command.c_str());
      
   yyin_cpp_popen (filename);
   DEBUGF ('m', "filename = %s, yyin = %p, fileno (yyin) = %d\n",
          filename, yyin, fileno (yyin));
   scanner_newfilename (filename);
   
   

   string outputfile= argv[argc-1];
   string outputtok=argv[argc-1];
   string outputast=argv[argc-1];
   string outputsym=argv[argc-1];

   outputfile=changeSuffix(outputfile, "str");
   outputtok=changeSuffix(outputfile, "tok");
   outputast=changeSuffix(outputfile, "ast");
   outputsym=changeSuffix(outputfile, "sym");

   outstr = fopen(outputfile.c_str(), "w");
   outtok = fopen(outputtok.c_str(), "w");
   outast = fopen(outputast.c_str(), "w");
   outsym = fopen(outputsym.c_str(), "w");

   yyparse();
   ast_tavesal(outsym,yyparse_astree);
   dump_astree (outast, yyparse_astree);
   
   cpplines (yyin, filename);
   
   yyin_cpp_pclose();
   DEBUGSTMT ('s', dump_stringset (stderr); );
   yylex_destroy();
   dump_stringset (outstr);
  
   free_ast (yyparse_astree);
   //dump_stringset (out2);
   fclose(outstr);
   fclose(outtok);
   fclose(outast);
   fclose(outsym);
  
   return get_exitstatus();
}


