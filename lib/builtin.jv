module builtin

import io
import struct

def str(input) {
  if ~input return 'None'
  if input is Object return input.to_s()
  __stringify(input)  ; Native C function
}

def cat(args) {
  result = ''
  if ~(args is Tuple) {
    result.extend(str(args))
    return result
  }
  for i=0, i < args.len(), i=i+1 {
    result.extend(str(args[i]))
  }
  return result
}

def hash(x) {
  if x is Object {
    return x.hash()
  }
  return Int(x)
}

class Object {
  method to_s() {
    return cat(
        'Instance of ',
        self.class().module().name(),
        '.',
        self.class().name())
  }
}

class Module {
  method to_s() {
    return cat('Module(', name(), ')')
  }
}

class Class {
  method to_s() {
    return cat('class ', name())
  }
}

class String {
  method to_s() {
    return self
  }
  method join(strs) {
    result = ''
    if strs.len() == 0 {
      return result
    }
    result.extend(str(strs[0]))
    for i=1, i<strs.len(), i=i+1 {
      result.extend(self)
      result.extend(str(strs[i]))
    }
    return result
  }
  method substr(start_index, num_chars=None) {
    if ~num_chars {
      num_chars = len() - start_index
    }
    end_index = start_index + num_chars
    if start_index <= end_index {
      return __substr(start_index, end_index)
    }
    return __substr(end_index, start_index)
  }
  method find(sub, index=0) {
    __find(sub, index)
  }
  method find_all(sub, index=0) {
    __find_all(sub, index)
  }
  method __in__(sub) {
    find(sub) != None
  }
}

class Tuple {
  method ==(other) {
    if (len() != other.len()) return False
    for i=0, i<len(), i=i+1 {
      if (self[i] != other[i]) {
        return False
      }
    }
    return True
  }
  method !=(other) {
    if (len() != other.len()) return True
    for i=0, i<len(), i=i+1 {
      if (self[i] != other[i]) {
        return True
      }
    }
    return False
  }
  method to_s() {
    result = '('
    result.extend(','.join(self))
    result.extend(')')
    return result
  }
  method iter() {
    return IndexIterator(self, 0, len())
  }
}

class Array {
  method ==(other) {
    if (len() != other.len()) return False
    for i=0, i<len(), i=i+1 {
      if (self[i] != other[i]) {
        return False
      }
    }
    return True
  }
  method !=(other) {
    if (len() != other.len()) return True
    for i=0, i<len(), i=i+1 {
      if (self[i] != other[i]) {
        return True
      }
    }
    return False
  }
  method to_s() {
    result = '['
    result.extend(','.join(map((elt) {
      if (elt is String) {
        return '\''.extend(elt).extend('\'')
      } else {
        return str(elt)
      }
    })))
    return result.extend(']')
  }
  method each(fn) {
    for i=0, i<len(), i=i+1 {
      fn(self[i])
    }
  }
  method map(fn) {
    result = []
    if len() == 0 {
      return result
    }
    result[len()-1] = None
    for i=0, i<len(), i=i+1 {
      result[i] = fn(self[i])
    } 
    return result
  }
  method iter() {
    return IndexIterator(self)
  }
}

class Function {
  method to_s() {
    if self.is_method() {
      return cat(
          'Method(',
          self.module().name(),
          '.', self.parent_class().name(),
          '.', self.name(), ')')
    } else {
      return cat('Function(', self.module().name(), '.', self.name(), ')')
    }
  }
}

class FunctionRef {
  method to_s() {
    cat('FunctionRef(obj=', self.obj(), ',func=', self.func(), ')')
  }
}

class Iterator {
  new(field has_next, field next) {}
  method iter() {
    return self
  }
}

class IndexIterator : Iterator {
  field indexable, start, end, i, index
  new(args) {
    if (args is Array) or (args is String) {
      indexable = args
      start = 0
      end = args.len()
    } else if args is Tuple {
      (indexable, start, end) = args
    } else {
      raise Error(concat('Strange input: ', args))
    }
    i = -1
    index = start - 1
    super()(
        () -> index < (end - 1),
        () {
          i = i + 1
          index = index + 1
          return (i, indexable[index])
        })
  }
}

class KVIterator : Iterator {
  new(field key_iter, field container) {
    super()(
        key_iter.has_next,
        () {
          k = key_iter.next()[1]
          return (k, container[k]) ; return required.
        })
  }
}

class Range {
  method to_s() {
    cat(start(), ':', inc(), ':', end())
  }
}

def range(params) {
  start = params[0]
  if params.len() == 3 {
    inc = params[1]
    end = params[2]
  } else {
    inc = 1
    end = params[1]
  }
  Range(start, inc, end)
}

def memoize(fn) {
  cache = {}
  memoized_fn = (args) {
    result = cache[args]
    if ~result {
      result = fn(args)
      cache[args] = result
    }
    return result
  }
  return memoized_fn
}

def _partition(seq, l, h) {
  x = seq[h]
  i = l - 1
  for j = l, j < h, j = j + 1 {
    if seq[j] < x {
      i = i + 1
      tmp = seq[i]
      seq[i] = seq[j]
      seq[j] = tmp
    }
  }
  i = i + 1
  tmp = seq[i]
  seq[i] = seq[h]
  seq[h] = tmp
  return i - 1
}

def _quick_sort(seq, l, h) {
  if l < h {
    p = _partition(seq, l, h)
    _quick_sort(seq, l, p)
    _quick_sort(seq, p + 1, h)
  }
  seq
}

def sort(seq) {
  _quick_sort(seq, 0, seq.len()-1)
}