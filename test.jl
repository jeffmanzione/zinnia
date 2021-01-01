module test

import classes
import io

c = classes.factory.create_class('Test', self)
    .with_field('p1')
    .with_field('p3')
c.get_constructor()
    .with_parameter('p1')
    .with_parameter('p2', True)
    .with_parameter('p3', False, '1')
    .with_parameter('p4', True, '2')
    .with_statement('x = p3 + p4')
c.add_method('test1')
    .with_parameter('p5')
    .with_parameter('p6', '3')
    .with_statement('p1 = p5 + p6')
    .with_statement('p2 = p6 - p5')
    .as_async()
io.println(c)