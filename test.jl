module test

import error
import io
import struct

;__collect_garbage()

io.println([1,2,3,4,5].map(x -> x*x))
for (i, x) in [1, 2, 3, 4, 5] {
  io.println(i, x)
}

map = {
  'abc': 123,
  'def': 456,
  'ghi': 789
}

io.println(map)

io.println(['abc', 'def', 'ghi'])

fn = memoize((x) {
  io.println('Test', x)
  return x
})

io.println(fn('abc'))
io.println(fn('abc'))
io.println(fn('def'))
io.println(fn('def'))