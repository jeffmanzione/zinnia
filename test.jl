module test

import io
import test2

io.println('abcqqabcqqabc'.find('abc'))
io.println('abcqqabcqqabc'.find('abc', 1))
io.println('abcqqabcqqabc'.find('abc', 6))
io.println('abcqqabcqqabc'.find_all('abc'))

test = 'aqcdef'
test[1] = 'b'
io.println(test)

test2.test2('Yo')
