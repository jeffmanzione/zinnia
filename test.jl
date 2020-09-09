module test

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
io.println(s)
io.println(a[1])
io.println(z)
io.println(cat(a, 5, None))