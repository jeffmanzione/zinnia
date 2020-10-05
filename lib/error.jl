module error

import io

class Token {
  field text, line, col
  new(field text, field line, field col) {}

  method to_s() {
    return cat('Token(text=\'', text, '\',line=', line, ',col=', col, ')')
  }
}

class StackLine {
  method token() {
    return Token(__token())
  }
  method to_s() {
    tok = token()
    func = function()
    if func {
      if func.is_method() {
        return cat(module().name(), '.', func.parent_class().name(), '.', func.name(), '(', module().filename(), ':', tok.line + 1, ')')
      }
      return cat(module().name(), '.', func.name(), '(', module().filename(), ':', tok.line + 1, ')')
    }
    return cat(module().name(), '(', module().filename(), ':', tok.line + 1, ')')
  }
}

class Error {
  method to_s() {
    result = cat(self.class().name(), ': ', message, '\n')
    for i=0, i<stacktrace.len(), i=i+1 {
      result.extend('  ').extend(str(stacktrace[i])).extend('\n')  
    }
    return result
  }
}