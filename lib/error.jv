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
    result = ''.extend(module().name())
    if func {
      if func.is_method() {
        result.extend('.').extend(func.parent_class().name())
      }
      result.extend('.').extend(func.name())
    }
    return result.extend('(')
        .extend(module().filename())
        .extend(':')
        .extend(str(tok.line + 1))
        .extend(')')
  }
}

class Error {
  method to_s() {
    result = cat(self.class().name(), ': ', message, '\n')
    error_token = stacktrace[0].token()
    left_padding = _token_left_padding(error_token)
    result.extend(str(error_token.line + 1)).extend(':')
    result.extend(stacktrace[0].linetext())
    for i=0, i < left_padding, i=i+1 {
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

; Left padding should be '<chars for row num>:<token col>'
def _token_left_padding(token) {
  line_text_space = 1
  if token.line >= 9999 {
    line_text_space = 5
  } else if token.line >= 999 {
    line_text_space = 4
  } else if token.line >= 99 {
    line_text_space = 3
  } else if token.line >= 9 {
    line_text_space = 2
  }
  ; +1 for the colon.
  return token.col + line_text_space + 1
}