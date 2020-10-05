module io

self.OUT = FileWriter('__STDOUT__', True)
self.ERROR = FileWriter('__STDERR__', True)

class _FileInternal {
  field file
  new(fn, rw, a, binary) {
    mode = cat(rw)
    if binary {
      mode.extend('b')
    }
    if a {
      mode.extend('+')
    }
    file = __File(fn, mode)
  }
  method puts(s) file.__puts(s)
  method close() file.__close()
}

class FileWriter {
  field fi
  new(fn, append=False, binary=False) {
    fi = _FileInternal(fn, 'w', append, binary)
  }
  method write(s) fi.puts(s)
  method writeln(s) {
    fi.puts(s)
    fi.puts('\n')
  }
  method close() fi.close()
}

def fprint(f, a) {
  f.write(str(a))
}

def fprintln(f, a) {
  f.writeln(str(a))
}

def print(a) {
  fprint(OUT, a)
}

def println(a) {
  fprintln(OUT, a)
}

def error(a) {
  fprint(ERROR, a)
}

def errorln(a) {
  fprintln(ERROR, a)
}