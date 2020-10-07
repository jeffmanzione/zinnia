module test

import error
import io

class Test {
  new(field t) {}
  method ==(other) t == other.t
  ;method ><=(other) 
  method [](i) return t + i
  method []=(k, v) {}
  method to_s() cat('Test(', t, ')')
}

io.println([1, 2, 3, 4] == [1, 2, 3, 4])
io.println([1, 2, 3, 4] != [1, 2, 3, 4])
io.println([1, 2, 3, 4] == [5, 2, 3, 4])
io.println([1, 2, 3, 4] != [5, 2, 3, 4])

msg = 'Hey!'

[1,2,3,4].each(x -> io.println(msg))

