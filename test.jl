module test

import error
import io

def test(x) {
  if x == 1 {
    return 1
  }
  return x * test(x - 1)
}

class Test {
  field t
  new() {
    t = 2
  }
  method method1(x) {
    [method1, self.method1, test]
  }
  method bad() {
    aaa.bbb.ccc
  }
}

x = 5
y = 2
test(x)
x = x + y

v = 0
for i=5, i>0, i=i-1 {
  v = v + i
}

t = Test()


io.println(t.method1(x)[0](x))

a = [1, 2, 3, 4, 5]

z = 'Hello, world!'
s = str(a)

def bad_fn2() {
  Test().bad()
}

def bad_fn() {
  bad_fn2()
}

try {
  io.println(s)
  io.println(a[1])
  bad_fn()
  io.println(z)
  io.println(cat(a, 5, None))
} catch e {
  io.fprintln(io.ERROR, e)
}

io.println(Test(), a.map(x -> x*x))