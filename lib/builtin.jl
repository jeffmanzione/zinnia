module builtin

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

class Object {
  method to_s() {
    return cat(self.class().name(), '()')
  }
}

class Class {
  method to_s() {
    return 'Instance of Class'
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
}

class Tuple {
  method to_s() {
    result = '('
    result.extend(','.join(self))
    result.extend(')')
    return result
  }
}

class Array {
  method to_s() {
    result = '['
    result.extend(','.join(self))
    result.extend(']')
    return result
  }
}

class Function {
  method to_s() {
    if self.is_method() {
      return cat('Method(', self.module().name(), '.', self.parent_class().name(), '.', self.name(), ')')
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