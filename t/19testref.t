use Test::Simple tests => 14;
use strict;

package Fart::Fiddle;

sub new {
    my $class = shift || __PACKAGE__;
    #print "Creating new ", __PACKAGE__, ": $class\n";
    return bless {}, $class;
}

sub foo {
    my $self = shift;
    $self->{foo} = 'perl foo';
    #warn $self->{foo};
    return $self->{foo};
}

sub DESTROY {
    my $self = shift;
    #warn "$self destroyed";
    $::destroyed = 1;
}

package main;

use Inline Config => DIRECTORY => './blib_test';
use Inline::Python qw(py_eval py_call_function py_new_object);
use Inline Python => <<END;

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

def gen(): return A()

def pass_through(obj): return obj

def swallow(obj): return

def call_method(obj):
    return obj.foo()

END


our $destroyed = 0;
sub check_destruction {
    $destroyed = 0;
    shift->();
    return $destroyed == 1;
}

my $o;
ok(check_destruction(sub { py_eval( <<END ) }), 'Perl object created and destroyed in Python');
o = perl.Fart.Fiddle.new()
del o
END

sub perl_pass_through {
    return shift;
}

ok(check_destruction(sub { perl_pass_through(Fart::Fiddle->new) }), 'Perl object in Perl');

ok(check_destruction(sub { py_call_function('__main__', 'pass_through', Fart::Fiddle->new) }), 'pass_through in void context');

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

my $a = py_new_object('A', '__main__', 'A');
ok($a->isa('Inline::Python::Object'));
my $foo = $a->foo();
ok($foo eq 'foo', 'got foo from Python');

ok(check_destruction( sub {
    $a = py_new_object('A', '__main__', 'A', Fart::Fiddle->new);
    ok($a->isa('Inline::Python::Object'));
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
ok(1); # no segfault in previous line
