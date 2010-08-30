use Test::More tests => 10;
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

END

ok(my $sub = get_sub(), 'Got something from get_sub');
ok($sub->(), 'hello Python');
ok($sub = get_sub_with_arg(), 'Got a sub ref for a sub with arguments');
ok($sub->('hello Python'), 'hello Python');

ok(call_perl_sub(bless {}, 'Foo'), 'Could call Perl sub from Python');

ok($sub = get_sub_from_perl(bless {}, 'Foo'), 'Got a reference to a Perl method');
ok($sub->(), 'Perl sub got passed through successfully');

ok($sub = getattr_sub_from_perl(bless {}, 'Foo'), 'Got a reference to a Perl method via getattr');
ok($sub->(), 'Perl sub got passed through getattr successfully');

ok(pass_through(sub { return 1; }), 'Pass through of perl sub ref works');

package Foo;

sub testsub {
    return 1;
}
