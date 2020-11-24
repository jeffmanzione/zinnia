module test

import io

io.println('abcqqabcqqabc'.find('abc'))
io.println('abcqqabcqqabc'.find('abc', 1))
io.println('abcqqabcqqabc'.find('abc', 6))
io.println('abcqqabcqqabc'.find_all('abc'))

io.println(cat('Collected ', __collect_garbage(), ' objects.'))

test = 'aqcdef'
test[1] = 'b'
io.println(test)

io.println(cat('Collected ', __collect_garbage(), ' objects.'))
io.println(cat('Collected ', __collect_garbage(), ' objects.'))

io.println('abcqqabcqqabc'.find('abc'))
io.println('abcqqabcqqabc'.find('abc', 1))
io.println('abcqqabcqqabc'.find('abc', 6))
io.println('abcqqabcqqabc'.find_all('abc'))

io.println(cat('Collected ', __collect_garbage(), ' objects.'))
