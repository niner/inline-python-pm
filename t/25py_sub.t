use Test::More tests => 15;
use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';

use Inline Python => <<END;

def get_sub():
    return lambda: 'hello Python'

def get_sub_with_arg():
    return lambda x: x

def call_perl_sub(foo):
    sub = getattr(foo, 'testsub')
    return sub()

def get_sub_from_perl(foo):
    return foo.testsub

def getattr_sub_from_perl(foo):
    return getattr(foo, 'testsub')

def pass_through(sub):
    return sub

class PyFoo:
    def get_method(self):
        return self.test_method
    def test_method(self):
        return 'foo'

END

ok(my $sub = get_sub(), 'Got something from get_sub');
is($sub->(), 'hello Python');
ok($sub = get_sub_with_arg(), 'Got a sub ref for a sub with arguments');
is($sub->('hello Python'), 'hello Python');

ok(call_perl_sub(bless {}, 'Foo'), 'Could call Perl sub from Python');

ok($sub = get_sub_from_perl(bless {}, 'Foo'), 'Got a reference to a Perl method');
ok($sub->(), 'Perl sub got passed through successfully');

ok($sub = getattr_sub_from_perl(bless {}, 'Foo'), 'Got a reference to a Perl method via getattr');
ok($sub->(), 'Perl sub got passed through getattr successfully');

ok(pass_through(sub { return 1; }), 'Pass through of perl sub ref works');

ok(call_perl_sub(bless {}, 'Bar'), 'Call inherited Perl method via getattr');
ok($sub = getattr_sub_from_perl(bless {}, 'Bar'), 'Got a reference to an inherited Perl method via getattr');
ok($sub->(), 'Inherited Perl method got passed through getattr successfully');

my $py_foo = PyFoo->new;
ok(my $method = $py_foo->get_method);
is($method->(), 'foo', 'Reference to Python method works');

package Foo;

sub testsub {
    return 1;
}

package Bar;

use base qw(Foo);
