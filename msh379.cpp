/*
# ------------------------------
# starter.cc -- a starter program file for CMPUT 379
#     This file includes the following:
#     - Some commonly used header files
#     - Functions WARNING and FATAL that can be used to report warnings and
#       errors after system calls
#     - A function to clear a VT100 screen (e.g., an xterm) by sending
#        the escape sequence "ESC[2J" followed by "ESC[H"
#     - A function to test a C++ vector whose elements are instances
#       of a struct.
#
#     This program is written largely as a C program, with the exception of
#     using some STL (Standard Template Library) container classes, and
#     possibly the C++ "new" operator.
#     (These two features are probably all we need from C++ in CMPUT 379.)
#    
#  Compile with:  g++ starter.cc -o starter         (no check for warnings)
#  	          g++ -Wall starter.cc -o starter   (check for warnings)
#		  g++ -ggdb starter.cc -o starter   (for debugging with gdb)
#
#  Usage:  starter  stringArg  intArg	 (e.g., starter abcd 100)
#
#  Author: Prof. Ehab Elmallah (for CMPUT 379, U. of Alberta)
# ------------------------------
*/

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <ctime>
#include <cassert>
#include <cstring>
#include <cstdarg>             //Handling of variable length argument lists
#include <sys/time.h>

#include <unistd.h>		
#include <sys/types.h>
#include <sys/stat.h>	
#include <sys/wait.h>
#include <sys/times.h>
#include <fcntl.h>

#include <string>
#include <vector>
using namespace std;

// ------------------------------

#define MAXLINE 256

typedef struct { char city[MAXLINE]; int temp; } cityTemp_t;

// ------------------------------
// The WARNING and FATAL functions are due to the authors of
// the AWK Programming Language.

void WARNING (const char *fmt, ... )
{
    va_list  ap;
    fflush (stdout);
    va_start (ap, fmt);  vfprintf (stderr, fmt, ap);  va_end(ap);
}

void FATAL (const char *fmt, ... )
{
    va_list  ap;
    fflush (stdout);
    va_start (ap, fmt);  vfprintf (stderr, fmt, ap);  va_end(ap);
    fflush (NULL);
    exit(1);
}
// ------------------------------------------------------------
// clrScreen() -- send VT100 escape sequence to clear the screen
// 	          (works fine in some cases)

void clrScreen () {
     char   ESC=033;		// the ESC character
     printf ("%c[2J", ESC); printf ("%c[H", ESC);
}

// ------------------------------------------------------------

void test_vector ()
{
    unsigned int        i, count;	
    vector<cityTemp_t>  ct;

    ct.clear();		 // Important to reset size to zero

    cityTemp_t  data[] = { {"Victoria", 16}, {"Vancouver", 15}, {"Prince George", 8},
    		           {"Edmonton", 5},  {"Calgary", 12}, {"Saskatoon", 9},
			   {"Regina", 11}, {"Winnipeg", 11} };


    count=  sizeof(data)/sizeof(cityTemp_t); 			   

    for (i=0 ; i < count; i++) ct.push_back(data[i]);

    printf ("test_vector: city-temperature table: ");
    for (i=0; i < ct.size(); i++)
        printf("[%s, %d], ", ct[i].city, ct[i].temp);
    printf("\n");

}

// ------------------------------ 
int main(int argc, char *argv[]) {

    char     strArg[MAXLINE];
    int	     intArg;

    strcpy (strArg, "");    intArg= 0;

    clrScreen();

    // read argv[1] (a text string) and argv[2] (an integer)
    //
    if (argc >= 2)  strcpy (strArg, argv[1]);  	// string copy
    if (argc >= 3)  intArg= atoi(argv[2]);
    printf ("%s:  strArg= %s, intArg= %d \n", argv[0], strArg, intArg);


    test_vector();
  
    WARNING ("testing function WARNING [%s] [%d]\n", strArg, intArg);
}

