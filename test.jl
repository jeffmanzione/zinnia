module test

import io

def test1() async {
  io.println('test1')
}
def test2() async {
  io.println('test2')
}

def test3() async {
  io.println('test3')
}

io.println(test1())

io.println('A')

test2()

io.println('B')

test3()

io.println('C')

def add(x, y) async {
  return x + y
}

io.println(await add(3.1412, 50.1))