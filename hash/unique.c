/*
** By Bob Jenkins, February 22, 1997
** This is an example of how to use the hash table.
**
** Given an input (stdin) with lines in any order
**   produce an output (stdout) with duplicate lines removed.
** Lines may not be longer than 4096 characters.
*/


#ifndef STANDARD
#include "standard.h"
#endif
#ifndef HASHTAB
#include "hashtab.h"
#endif
#define LINELEN 4096


int main()
{
  ub1   buf[LINELEN];
  ub1 *key;
  ub4  keyl;
  htab *t;
  htab *t2;
  int  i;
  FILE *fp;


  fp = fopen("samperf.txt", "r");

  t = celhcreate(8);                      /* create hash table */
  t2 = celhcreate(8);                     /* create hash table */

  i = 0;

  /* read in all the lines */
  while (fgets((char *)buf,LINELEN,fp))            /* get line from stdin */
  {
    keyl = strlen((char *)buf);
    buf[(keyl-1)] = 0;
    if (hadd(t, buf, keyl, (void *)0)) /* if not a duplicate */
    {
      key = (ub1 *)malloc(keyl);       /* dumb use of malloc */
      strcpy(key, buf);                /* copy buf into key */
      hkey(t)=key;                     /* replace buf with key */
      haddInt(t2, i, key);
      printf(" i-in: %d [%s]\n",i,key); /* dump it to stdout */
      i++;
    }
  }

  for (i = 0; ; i++)
  {
     if (hfindInt(t2, i))
     {
        printf("i-out: %d [%s]\n", i, hstuff(t2)); /* dump it to stdout */
     } else {
        printf("i-out: Stop\n");
       break;
     }
  }

  if (hfirst(t)) do                    /* go to first element */
  {
    printf("%.*s\n",hkeyl(t),hkey(t)); /* dump it to stdout */
  }
  while (hnext(t));                    /* go to next element */

  while (hcount(t))                    /* while the table is not empty */
  {
    free(hkey(t));                     /* free memory for the line */
    hdel(t);                           /* delete from hash table */
  }

  hdestroy(t);                         /* destroy hash table */
  hdestroy(t2);                         /* destroy hash table */

  return SUCCESS;
}
