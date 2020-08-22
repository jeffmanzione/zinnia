module test

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
  method method1(x) { x + t }
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

t.method1(x)