use Test::Simple tests => 57;
use strict;

package Fart::Fiddle;

sub new {
    my $class = shift || __PACKAGE__;
    $class = ref $class if ref $class;
    #print "Creating new ", __PACKAGE__, ": $class\n";
    $::destroyed--;
#    warn $::destroyed;
    return bless {}, $class;
}

sub foo {
    my $self = shift;
    $self->{foo} = 'perl foo';
    #warn $self->{foo};
    return $self->{foo};
}

sub b {
    return B->new;
}

sub DESTROY {
    my $self = shift;
    #warn "$self destroyed";
    $::destroyed++;
#    warn $::destroyed;
}

sub more {
    my $self = shift;
    return ( $self->new, $self->new );
}

sub more_ref {
    my $self = shift;
    return [ $self->new, $self->new ];
}

sub more_hash {
    my $self = shift;
    return { a => $self->new, b => $self->new };
}

sub bark {
    my $self = shift;
    return "bark: $self";
}

sub pass_through {
    my $self = shift;
    return @_;
}

package main;

sub named {
    my ($positional, $named) = @_;
}

use Inline Config => DIRECTORY => './blib_test';
use Inline::Python qw(py_eval py_call_function py_new_object);
use Inline Python => <<END;

import perl
import sys
import gc

class A:
    def __init__(self, obj = None):
        self.data = {'obj': obj}

    def foo(self):
        self.data['foo'] = 'foo'
        return self.data['foo']

    def obj_foo(self):
        return self.data['obj'].foo()

    def obj(self):
        return self.data['obj']

class B:
    def __init__(self):
        perl.raise_cnt()

    def __del__(self):
        perl.lower_cnt()

    def foo(self):
        return 'foo'

def gen(): return A()

def pass_through(obj): return obj

def swallow(obj): return

def call_method(obj):
    return obj.foo()

def call_methods(obj_list):
    for obj in obj_list:
        obj.bark()

def give_list(obj):
    return (obj.new(), obj.new())

def give_array(obj):
    arr = []
    arr.append(obj)
    return arr

def give_hash(obj):
    return {'a': obj.new(), 'b': obj.new()}

def call_more(obj):
    more = obj.more()
    perl.ok(len(more) > 0, 'obj.more gave values')
    for o in more:
        o.bark()

def call_more_ref(obj):
    more = obj.more_ref()
    perl.ok(len(more) > 0, 'obj.more_ref gave values')
    for o in more:
        o.bark()

def call_more_hash(obj):
    obj.more_hash()

def call_method_params(obj, param):
    perl.ok(obj.pass_through(param) == param, 'got param back from pass_through')

def call_method_param_array(obj):
    obj.pass_through([obj.new(), obj.new()])

def call_method_param_hash(obj):
    obj.pass_through({'a': obj.new(), 'b': obj.new()})

def test_exec(code, *args, **kwargs):
    try:
        exec code
        res = test_func(*args, **kwargs)
        if res == sys.stdout:
            return "foo"
        return res
    except Exception, e:
        raise Exception(str(e) + ' at line ' + str(sys.exc_traceback.tb_next and sys.exc_traceback.tb_next.tb_lineno or sys.exc_traceback.tb_lineno))
    finally:
        try:
            del(test_func)
        except:
            pass

def call_named(obj, foo):
    perl.named(obj, foo=foo)

def py_obj_from_perl_obj(obj):
    return obj.b().foo()

END


our $destroyed = 1;
sub check_destruction {
    $destroyed = 1;
    shift->();
    return $destroyed == 1;
}
sub raise_cnt {
    $destroyed--;
}
sub lower_cnt {
    $destroyed++;
}

my $o;
ok(check_destruction(sub { py_eval( <<END ) }), 'Perl object created and destroyed in Python');
o = perl.Fart.Fiddle.new()
del o
END

sub perl_pass_through {
    return shift;
}

ok(check_destruction(sub { perl_pass_through(Fart::Fiddle->new) }), 'Perl object in Perl'); # this is more a test of check_destruction itself

ok(check_destruction(sub { py_call_function('__main__', 'pass_through', Fart::Fiddle->new) }), 'pass_through in void context');

ok(check_destruction(sub { py_call_function('__main__', 'B') }), 'Python object constructor');
ok(check_destruction(sub { py_eval('B()', 0) }), 'Python object constructor in py_eval');

ok(check_destruction(sub { py_call_function('__main__', 'pass_through', B->new) }), 'pass_through of Python object in void context');
ok(check_destruction(sub { my $b = B->new; $b->foo(); py_call_function('__main__', 'swallow', $b) }), 'swallow of Python object');
ok(check_destruction(sub { my $b = B->new; py_call_function('__main__', 'py_obj_from_perl_obj', Fart::Fiddle->new) }), 'Python function gets a Python object from a Perl object');


ok(check_destruction(sub {
    my $o = py_call_function('__main__', 'pass_through', Fart::Fiddle->new);
    #warn $o;
    $o->foo;
    #warn "undefing";
    undef $o;
}), 'pass_through with return value');
#warn "swallow";

ok(check_destruction(sub { py_call_function('__main__', 'swallow', Fart::Fiddle->new) } ), 'swallow');

ok(check_destruction(sub { py_call_function('__main__', 'call_method', Fart::Fiddle->new) } ), 'call_method');

ok(check_destruction( sub { py_call_function('__main__', 'swallow', [Fart::Fiddle->new, Fart::Fiddle->new]) } ), 'swallow array ref');

ok(check_destruction( sub { py_call_function('__main__', 'call_methods', [Fart::Fiddle->new, Fart::Fiddle->new]) } ), 'call_methods on array ref');

ok(py_call_function('__main__', 'pass_through', 'foo') eq 'foo', 'simple string pass through');
ok(py_call_function('__main__', 'pass_through', 10) == 10, 'simple integer pass through');

ok(check_destruction( sub {
    my $list = py_call_function('__main__', 'give_list', Fart::Fiddle->new);
    my $foo = "$list"; # just to deref
    $foo = "@$list";
} ), 'Python list to perl');

ok(check_destruction( sub {
    my @list = py_call_function('__main__', 'give_list', Fart::Fiddle->new);
    my $foo = "@list";
} ), 'Python list to perl in list context');

ok(check_destruction( sub {
    my @foo = @{ py_call_function('__main__', 'give_list', Fart::Fiddle->new) };
    ok(@foo == 2, 'list has two entries');
    ok($foo[0], 'list value is there');
} ), 'Python list to perl dereferenced immediately');

ok(check_destruction( sub {
    my @array = py_call_function('__main__', 'give_array', Fart::Fiddle->new);
    my $foo = "@array";
} ), 'Python list to perl in list context');

ok(check_destruction( sub {
    my $foo = py_call_function('__main__', 'give_hash', Fart::Fiddle->new);
    ok(values %$foo == 2, 'hash has entries');
    ok($foo->{a}, 'hash value is there');
} ), 'Python dict to perl');

ok(check_destruction( sub {
    my %foo = %{ py_call_function('__main__', 'give_hash', Fart::Fiddle->new) };
    ok((values %foo == 2 and $foo{a}), 'hash values seem ok');
} ), 'Python dict to perl dereferenced immediately');

ok(check_destruction( sub {
    my $list = py_call_function('__main__', 'pass_through', [Fart::Fiddle->new, Fart::Fiddle->new]);
    ok(ref $list eq 'ARRAY', 'got array ref back from python');
    ok((@$list == 2 and $list->[0] and $list->[1]), 'list still contains two elements');
    $list->[0]->bark();
} ), 'pass_through of an array ref');

ok(check_destruction( sub {
    py_call_function('__main__', 'pass_through', [Fart::Fiddle->new, Fart::Fiddle->new]);
} ), 'pass_through of an array ref');

my $a = py_new_object('A', '__main__', 'A');
ok($a->isa('Inline::Python::Object'), 'Python object created');
my $foo = $a->foo();
ok($foo eq 'foo', 'got foo from Python');

ok(check_destruction( sub {
    $a = py_new_object('A', '__main__', 'A', Fart::Fiddle->new);
    ok($a->isa('Inline::Python::Object'), 'Python object created with parameter');
    $foo = $a->obj_foo();
    ok($foo eq 'perl foo', 'got perl foo from Perl via Python');
    $o = $a->obj();
    ok($o->isa('Fart::Fiddle'), 'Perl object safely returned to perl');
    undef $a;
    $o->foo();
    undef $o;
} ), 'Perl object destroyed after python object');

ok(check_destruction( sub {
    $a = py_new_object('A', '__main__', 'A', Fart::Fiddle->new);
    undef $a;
} ), 'Perl object destroyed with python object');

$a = py_new_object('A', '__main__', 'A');
py_call_function('__main__', 'call_method', $a);
ok(1, 'no segfault on call_method'); # no segfault in previous line

ok(check_destruction( sub { py_call_function('__main__', 'call_more', Fart::Fiddle->new) } ), 'perl function returns list of objects to python');
ok(check_destruction( sub { py_call_function('__main__', 'call_more_ref', Fart::Fiddle->new) } ), 'perl function returns array of objects to python');
ok(check_destruction( sub { py_call_function('__main__', 'call_more_hash', Fart::Fiddle->new) } ), 'perl function returns hash of objects to python');

ok(check_destruction( sub { py_call_function('__main__', 'call_method_params', Fart::Fiddle->new, 'foo') } ), 'perl function with scalar params called from python');
ok(check_destruction( sub { py_call_function('__main__', 'call_method_params', Fart::Fiddle->new, Fart::Fiddle->new) } ), 'perl function with object params called from python');
ok(check_destruction( sub { py_call_function('__main__', 'call_method_param_array', Fart::Fiddle->new) } ), 'perl function with param array called from python');
ok(check_destruction( sub { py_call_function('__main__', 'call_method_param_hash', Fart::Fiddle->new) } ), 'perl function with param hash called from python');

ok(check_destruction( sub {
    my $obj = py_new_object('A', '__main__', 'A', [Fart::Fiddle->new, Fart::Fiddle->new]);
    my $list = $obj->obj();
    ok((@$list == 2 and $list->[0]), 'array values look ok');
} ), 'method returned array ref');

my $test_func = <<'TEST_FUNC';
def test_func(context):
    foo = context['foo']
    context['bar'] = foo.new()
    return foo
TEST_FUNC

ok(check_destruction( sub { py_call_function('__main__', 'test_exec', $test_func, {foo => Fart::Fiddle->new}) } ), "exec'ed and deleted function");

$test_func = <<'TEST_FUNC';
def test_func(context):
    arr = []
    def find_foo(obj, i):
        if i == 0:
            find_foo(obj, 1)
        arr.append(obj)
    find_foo(context, 0)
    return arr
TEST_FUNC

ok(check_destruction( sub {
    my @arr = py_call_function('__main__', 'test_exec', $test_func, Fart::Fiddle->new);
    py_call_function('gc', 'collect'); # Kick the GC. Otherwise the find_foo call frame would still reference arr
} ), "exec'ed and deleted function");

ok(check_destruction( sub { call_named(Fart::Fiddle->new, Fart::Fiddle->new) }), 'Object passed as positional and named parameter deleted');
ok(check_destruction( sub { call_named(0, 0) }), 'Scalar passed as positional and named parameter deleted');

ok(check_destruction(sub { my $o; py_call_function('__main__', 'swallow', sub { $o = Fart::Fiddle->new }) } ), 'swallow perl sub with object ref');
ok(check_destruction(sub { my $b = B->new; py_call_function('__main__', 'swallow', sub { return $b }) }), 'swallow of Python object');
