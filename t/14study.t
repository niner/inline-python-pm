use Test;
BEGIN { plan tests => 14 }
use Inline::Python qw(py_eval py_study_package);

py_eval(<<END);
def Foo(): pass
def bar(): pass
class bone: pass
class zone: pass
class comp:
    def __init__(self): pass
    def foo(self): pass
    def bar(self): pass
END

%n = py_study_package; # defaults to __main__

# Only two elements: functions and classes
ok(scalar keys %n, 2);
ok(defined $n{functions});
ok(defined $n{classes});

# Two functions: Foo and bar
ok(scalar @{$n{functions}}, 2);
@fns = sort @{$n{functions}};
ok($fns[0], "Foo");
ok($fns[1], "bar");

# Three classes: bone, zone and comp
ok(scalar keys %{$n{classes}}, 3);
@cls = sort keys %{$n{classes}};
ok($cls[0], 'bone');
ok($cls[1], 'comp');
ok($cls[2], 'zone');

# 'comp' has three methods: __init__, bar, and foo
ok(scalar @{$n{classes}{comp}}, 3);
@fns = sort @{$n{classes}{comp}};
ok($fns[0], '__init__');
ok($fns[1], 'bar');
ok($fns[2], 'foo');
