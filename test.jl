module test

import io
import process

io.println('Hi')

def count(n) {
  io.println('Yo', n)
  i = 0
  for i=0, i<n, i=i+1 {
    io.println(i)
  }
}
io.println('Hi')

process.create_process(count, 100)

for i=100, i>0, i=i-1 {
  io.println(i)
}

io.println('Hi')