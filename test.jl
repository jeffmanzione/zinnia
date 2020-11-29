module test

import io

;io.println('abcqqabcqqabc'.find('abc'))
;io.println('abcqqabcqqabc'.find('abc', 1))
;io.println('abcqqabcqqabc'.find('abc', 6))
;io.println('abcqqabcqqabc'.find_all('abc'))

;io.println(cat('Collected ', __collect_garbage(), ' objects.'))

;test = 'aqcdef'
;test[1] = 'b'
;io.println(test);

def add1(x, y) async {
  io.println('add1')
  x + y
}
add2 = (x, y) async {
  io.println('add2')
   x + y
}

add3 = (x, y) async {
  io.println('add3')
   x + y
}

io.println('A', await add1(1, 2))
io.println('B')
add2(3, 4)
io.println('C')
add3(5, 6)
io.println('D')