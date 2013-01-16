//this version of tail will support -f (follow) flag
//only follows up to one file

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

enum {
  LOOPSLEEP = 2,
  BUFSIZE = 1000
};

int
main (int argc, char **args) {
  bool follow;
  follow = false;
  int fd;
  char *filepath;
  fd = 0;

  if (argc > 1) {
    int i;
    for (i = 1; i < argc; ++i) {
      if (strcmp("-f", args[i]) == 0) {
        follow = true;
      }
      else if (fd == 0) {
        fd = open(args[i], O_RDONLY, 0); 
        filepath = args[i];
      } 
    } 
  }
  
  if (fd > -1) {
    struct stat sbuf;
    fstat(fd, &sbuf);
    off_t size = sbuf.st_size;
    off_t remsize = size;
    
    int nls = 0;
    lseek(fd, 0, SEEK_END);
    char c;
    char buf[100];
    
    while (size > 0 && nls < 11) {
      lseek(fd,--size,SEEK_SET);
      if (read(fd, &c, 1) > 0) {
        if (c == '\n') {
          nls++;
        }
      }
      else {
        break;
      }
    }
    
    if (nls >= 11) {
      ++size;  
    }
    lseek(fd, size, SEEK_SET);
    
    int diff = remsize - size;
    int incr = (diff < BUFSIZE) ? diff : BUFSIZE;
    for (; read(fd, buf, incr) > 0;) {
      write(1, buf, diff);
      diff -= incr;
      incr = (diff < BUFSIZE) ? diff : BUFSIZE;
    }

    if (follow) {
      int readr;
      while (1) {
        //http://cow.physics.wisc.edu/~craigm/idl/archive/msg00399.html
        // - seems to be an issue with many file systems; local ext3 on ubuntu 12.04 also caches fstat
        // - fstat may not return newest file size in files contained in NFS
        // - teach me to dev on ec2
        // no dice fchown(fd, sbuf.st_uid, sbuf.st_gid);
        // no dice fcntl
        if (fd > 0) {
          close(fd);
          fd = open(filepath, O_RDONLY, 0);
        }
        
        struct stat sbuf2;
        fstat(fd, &sbuf2);
        if (remsize < sbuf2.st_size) {
          diff = sbuf2.st_size - remsize;
          lseek(fd, --remsize, SEEK_SET);
          while (diff > 0) {
            incr = (diff < BUFSIZE) ? diff : BUFSIZE;
            int r = read(fd, buf, incr);
            diff -= incr;
            write(1, buf, incr);
          }
          remsize = sbuf2.st_size;
        } 
        sleep(LOOPSLEEP); 
      }
    }
     
  }
  else {
    exit(1);  
  }
  exit(0);
}

