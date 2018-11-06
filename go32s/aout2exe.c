#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <alloc.h>
#include <string.h>

#include "stubbytes.h"

main(int argc, char **argv)
{
  int i;
  for (i=1; i<argc; i++)
    aout2exe(argv[i]);
}

aout2exe(char *fname)
{
  int ifile;
  int ofile;
  char *ofname;
  char buf[4096];
  int rbytes;
  long hsize;

  ifile = open(fname, O_RDONLY|O_BINARY);
  if (ifile < 0)
  {
    perror(fname);
    return;
  }

  ofname = strrchr(fname, '/');
  if (ofname == 0)
    ofname = strrchr(fname, '\\');
  if (ofname == 0)
    ofname = fname;
  ofname = strrchr(ofname, '.');
  if (ofname) *ofname = 0;

  ofname = (char *)malloc(strlen(fname)+5);
  sprintf(ofname, "%s.exe", fname);
  ofile = open(ofname, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666);
  if (ofile < 0)
  {
    perror(ofname);
    free(ofname);
    return;
  }
  hsize = (sizeof(stub_bytes) + 511) & ~511;
  ((short *)stub_bytes)[2] = hsize / 512;
  ((short *)stub_bytes)[1] = 0;
  write(ofile, stub_bytes, hsize);
  
  while ((rbytes=read(ifile, buf, 4096)) > 0)
  {
    int wb = write(ofile, buf, rbytes);
    if (wb < 0)
    {
      perror(ofname);
      break;
    }
    if (wb < rbytes)
    {
      fprintf(stderr, "%s: disk full\n", ofname);
      exit(1);
    }
  }
  close(ifile);
  close(ofile);
  free(ofname);
/*  remove(fname); */
}
