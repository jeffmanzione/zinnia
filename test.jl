module test

import classes
import io

c = classes.factory.create_class('Test', self)
    .with_field('b')
c.get_constructor()
    .with_parameter('a', True, '\'a\'')
    .with_parameter('b', False)
    .with_parameter('p4', True, '2')
    .with_statement('x = p3 + p4')
c.add_method('set_b')
    .with_parameter('new_b')
    .with_statement('b = new_b')
c.add_method('to_s')
    .with_statement('cat(\'Test(\a=\', a, \', b=\', b, \')\')')
io.println(c)

T = c.build()

;io.println(T)
;io.println(Test)