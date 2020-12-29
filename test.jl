module test

;import async
import io
import struct

;do_something = (c) async -> c.complete('Hello, world!')
;completer = async.Completer()
;do_something(completer)
;io.println(await completer.as_future())
;io.println(await async.value('Hello').then(x -> cat(x, ', world!')))

m1 = {
  'a': 1,
  'b': 2,
  'c': 3
}
m2 = m1.copy()
m2['d'] = 4

io.println(m1)
io.println(m2)


Test().start()