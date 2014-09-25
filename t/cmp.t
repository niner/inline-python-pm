package Comparer;

sub new {
    my ($class, $value) = @_;
    return bless \$value;
}

sub __cmp__ {
    my ($self, $other) = @_;
    return $$self cmp $$other;
}

sub __eq__ {
    my ($self, $other) = @_;
    return $$self cmp $$other;	
}

package FailComparer;

sub new {
    my ($class, $value) = @_;
    return bless \$value;
}

sub __cmp__ {
    my ($self, $other) = @_;
    return 'foo';
}

sub __eq__ {
    my ($self, $other) = @_;
    return 'foo';
}

package main;

use Test::More;
use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';

BEGIN { plan tests => 7 }

use Inline::Python qw(py_eval py_call_function);

py_eval(<<'END');

def test(foo1, foo2, bar):
    perl.ok(foo1 == foo1);
    perl.ok(foo2 == foo2);
    perl.ok(bar  == bar);
    perl.ok(foo1 == foo2, 'foo1 == foo2');
    perl.ok(foo1 != bar);
    perl.ok(foo2 != bar);

def test_fail(o1, o2):
    o1 == o2

END

py_call_function("__main__", "test", map { Comparer->new($_) } qw(foo foo bar));
eval {
    py_call_function("__main__", "test_fail", map { FailComparer->new($_) } qw(foo foo));
};
like($@, qr/__ must return an integer!/);
