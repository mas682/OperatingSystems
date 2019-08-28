// program used to read through a file that contains numbers only
// and state if next value larger than previous
// used for project 3 1550
// matt stropkey


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
  FILE *in_file;
  in_file = fopen(argv[1], "r");
  int val = 0;
  int previous = 200000;
  int i = 2;
  while(fscanf(in_file, "%d\n", &val) == 1)
  {
    printf("%d\n", val);
    if(val > previous)
    {
      printf("PAGE NUMBER %d larger than previous\n", i);
    }
    previous = val;
    i++;
  }
  exit(0);
}
