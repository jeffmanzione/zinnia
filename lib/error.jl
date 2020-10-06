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
    error_token = stacktrace[0].token()
    result.extend(str(error_token.line + 1)).extend(':')
    result.extend(stacktrace[0].linetext())
    for i=0, i < error_token.col + error_token.line % 10 + 1, i=i+1 {
      result.extend(' ')
    }
    for i=0, i < error_token.text.len(), i=i+1 {
      result.extend('^')
    }
    result.extend('\n')
    for i=0, i<stacktrace.len(), i=i+1 {
      result.extend('  ').extend(str(stacktrace[i])).extend('\n')  
    }
    return result
  }
}