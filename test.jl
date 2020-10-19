module test

import error
import io
import struct



def test1() {
  a = 1
  tmp = (x -> x + a)
  io.println(tmp)
  io.println(tmp(2))
}

;__collect_garbage()

io.println([1,2,3,4,5].map(x -> x*x))

test1()