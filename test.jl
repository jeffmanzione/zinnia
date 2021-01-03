module test

import classes
import io

c = classes.factory.create_class('Test', self)
    .with_field('b')
c.get_constructor()
    .with_parameter('a', True, '1')
    .with_parameter('b', True)
    .with_statement('x = a + b')
    .with_statement('io.println(x)')
c.add_method('set_b')
    .with_parameter('new_b')
    .with_statement('b = new_b')
c.add_method('to_s')
    .with_statement('cat(\'Test(a=\', a, \', b=\', b, \')\')')
io.println(c)

io.println(c.build())
io.println(Test)

io.println(Class.methods())

a = [1, 2, 3, 4, 5]
io.println(a)