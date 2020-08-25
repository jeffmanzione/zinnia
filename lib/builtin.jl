module builtin

def str(input) {
  if ~input return 'None'
  if input is Object return input.to_s()
  stringify__(input)  ; External C function
}

class Object {
  method to_s() {
    return 'Instance of Object'
  }
}

class Class {
  method to_s() {
    return 'Instance of Class'
  }
}
