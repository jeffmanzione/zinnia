#include <stdlib.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "run/run.h"
#include "struct/alist.h"
#include "util/args/commandline.h"
#include "util/args/commandlines.h"

const char LIB_fibonacci[] =
  "module fibonacci\nsource \'.\\examples\\simple\\fibonacci.jp\'\n  lmdl  io       #2 7\n  lmdl  struct   #3 7\n  push  memoize  #6 6\n  jmp   21       #6 14\n@$anon_6_14\n  let   i        #6 15\n  eq    0        #7 13\n  if    3        #7 16\n  push  i        #7 20\n  push  1        #7 25\n  eq             #7 22\n  ifn   2        #7 4\n  res   1        #8 13\n  ret            #8 6\n  push  fib     "
  " #10 11\n  res   i        #10 15\n  sub   1        #10 19\n  call           #10 14\n  push           #10 22\n  push  fib      #10 24\n  res   i        #10 28\n  sub   2        #10 32\n  call           #10 27\n  push           #10 22\n  add            #10 22\n  ret            #10 4\n  res   $anon_6_14 #6 14\n  call           #6 13\n  set   fib      #6 0\n  push  io       #13 0\n  push  cat      #13"
  " 11\n  push  fib      #13 26\n  res   5        #13 30\n  call           #13 29\n  push           #13 24\n  push  \'fib(5)=\' #13 23\n  tupl  2        #13 24\n  call           #13 14\n  call  println  #13 3\n  push  io       #14 0\n  push  cat      #14 11\n  push  fib      #14 27\n  res   10       #14 31\n  call           #14 30\n  push           #14 25\n  push  \'fib(10)=\' #14 24\n  tupl  2      "
  "  #14 25\n  call           #14 14\n  call  println  #14 3\n  push  io       #15 0\n  push  cat      #15 11\n  push  fib      #15 27\n  res   20       #15 31\n  call           #15 30\n  push           #15 25\n  push  \'fib(20)=\' #15 24\n  tupl  2        #15 25\n  call           #15 14\n  call  println  #15 3\n  push  io       #16 0\n  push  cat      #16 11\n  push  fib      #16 27\n  res   30     "
  "  #16 31\n  call           #16 30\n  push           #16 25\n  push  \'fib(30)=\' #16 24\n  tupl  2        #16 25\n  call           #16 14\n  call  println  #16 3\n  push  io       #17 0\n  push  cat      #17 11\n  push  fib      #17 27\n  res   40       #17 31\n  call           #17 30\n  push           #17 25\n  push  \'fib(40)=\' #17 24\n  tupl  2        #17 25\n  call           #17 14\n  call  println"
  "  #17 3\n  exit  0\nbody\n \'; Example program which efficiently computes fibonacci numbers using memoize().\\n\'\n \'\\n\'\n \'import io\\n\'\n \'import struct\\n\'\n \'\\n\'\n \'; fib(i) = fib(i-1) + fib(i-2)\\n\'\n \'fib = memoize((i) {\\n\'\n \'    if (i == 0) or (i == 1) {\\n\'\n \'      return 1\\n\'\n \'    }\\n\'\n \'    return fib(i - 1) + fib(i - 2)\\n\'\n \'  })\\n\'\n \'\\n\'\n \'io.println(cat(\\\'fib(5)=\\\',"
  " fib(5)))\\n\'\n \'io.println(cat(\\\'fib(10)=\\\', fib(10)))\\n\'\n \'io.println(cat(\\\'fib(20)=\\\', fib(20)))\\n\'\n \'io.println(cat(\\\'fib(30)=\\\', fib(30)))\\n\'\n \'io.println(cat(\\\'fib(40)=\\\', fib(40)))\'\n";

int main(int argc, const char *argv[]) {
  alloc_init();
  strings_init();
  ArgConfig *config = argconfig_create();
  argconfig_run(config);
  ArgStore *store = commandline_parse_args(config, argc, argv);
  AList srcs;
  alist_init(&srcs, char *, argc - 1);
  AList src_contents;
  alist_init(&src_contents, char *, argc - 1);
  *(char **)alist_add(&srcs) = ".\\examples\\simple\\fibonacci.jp";  *(char **)alist_add(&src_contents) = LIB_fibonacci;  run_files(&srcs, &src_contents, store);
  alist_finalize(&srcs);
  alist_finalize(&src_contents);
#ifdef DEBUG
  argstore_delete(store);
  argconfig_delete(config);
  strings_finalize();
  token_finalize_all();
  alloc_finalize();
#endif
  return EXIT_SUCCESS;
}
