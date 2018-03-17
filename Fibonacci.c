#include <stdio.h>

// used as source code to be compiled
 
int main()
{
   int n, first = 0, second = 1, next, c;
 
   n = 100000000;
 
   for ( c = 0 ; c < n ; c++ )
   {
      if ( c <= 1 )
         next = c;
      else
      {
         next = first + second;
         first = second;
         second = next;
      }
      //printf("%d\n",next);
   }
 
   return 0;
}

