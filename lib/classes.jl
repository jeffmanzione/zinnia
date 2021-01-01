module classes

import struct

class _MethodBuilder {
  field statements, parameters, is_async

  new(field method_name) {
    statements = []
    parameters = []
    is_async = False
  }

  method with_parameter(name, default_value = None) {
    parameters.append((name, default_value))    
    return self
  }

  method as_async() {
    is_async = True
    return self
  }
  
  method with_statement(statement_text) {
    statements.append(statement_text)
    return self
  }
  
  method to_s() {
    c_text = cat(
      '  method ',
      method_name,
      '(',
      ', '.join(parameters.map(
          (f) {
            f_text = f[0].copy()
            if f[1] {
              f_text.extend(' = ').extend(str(f[1]))
            }
            return f_text
          })),
      ') {\n')
    c_text.extend('\n'.join(statements.map(s -> cat('    ', s))))
    c_text.extend('\n  }\n')
    return c_text
  }
}

class _ConstructorBuilder {
  field parameters, statements
  
  new() {
    parameters = []
    statements = []
  }
  
  method with_parameter(name, is_field = False, default_value = None) {
    parameters.append((name, is_field, default_value))
    return self
  }
  
  method with_statement(statement_text) {
    statements.append(statement_text)
    return self
  }

  method to_s() {
    c_text = cat(
      '  new(',
      ', '.join(parameters.map(
          (f) {
            f_text = ''
            if f[1] {
              f_text.extend('field ')
            }
            f_text.extend(f[0])
            if f[2] {
              f_text.extend(' = ').extend(str(f[2]))
            }
            return f_text
          })),
      ') {\n')
    c_text.extend('\n'.join(statements.map(s -> cat('    ', s))))
    c_text.extend('\n  }\n')
    return c_text
  }
}

class _ClassBuilder {
  field fields, constructor, methods

  new(field class_name, field module) {
    fields = []
    constructor = None
    methods = []
  }

  method with_field(field_name) {
    fields.append(field_name)
    return self
  }

  method get_constructor() {
    if ~constructor {
      constructor = _ConstructorBuilder()
    }
    return constructor
  }

  method add_method(method_name) {
    m = _MethodBuilder(method_name)
    methods.append(m)
    return m
  }

  method build() {
    return __load_class_from_text(module, to_s().extend('\nNone\n'))
  }

  method to_s() {
    class_text = cat('class ', class_name, ' {\n')
    if fields.len() > 0 {
      class_text.extend(cat('  field ', ', '.join(fields), '\n'))
    }
    if constructor {
      class_text.extend(constructor.to_s())
    }
    for (_, m) in methods {
      class_text.extend(m.to_s())
    }
    class_text.extend('}\n')
  }
}

class _ClassFactory {
  method create_class(class_name, module) {
    return _ClassBuilder(class_name, module)
  }
}

self.factory = _ClassFactory()
