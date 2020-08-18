module test

def test(x) {
  if x == 1 {
    return 1
  }
  return x + test(x - 1)
}

x = 5
y = 2
test(x)
x = x + y