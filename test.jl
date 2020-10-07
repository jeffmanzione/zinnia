module test

import error
import io

class Test {
  new(field t) {}
  def =(other) t == other.t
  def to_s() cat('Test(', t, ')')
}

io.println(Test(1), Test(2))