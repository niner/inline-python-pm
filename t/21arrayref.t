use Test::More tests => 11;

use Inline Config => DIRECTORY => './blib_test';
use Inline::Python qw(py_call_function);
use Inline Python => <<'END';

def return_empty_array():
    return []

def return_onesized_array():
    return [1]

def bounce_array(a):
    return a

def perl_list(a):
    return a.list();

END

my $a = py_call_function('__main__', 'return_empty_array');
ok(ref $a eq 'ARRAY');
ok(@$a == 0, 'emtpy array ref -> empty list');
my @a = py_call_function('__main__', 'return_empty_array');
ok(@a == 0);

ok(ref scalar py_call_function('__main__', 'return_onesized_array') eq 'ARRAY');
@a = py_call_function('__main__', 'return_onesized_array');
ok(@a == 1);

ok(ref scalar py_call_function('__main__', 'bounce_array', [Foo->new]) eq 'ARRAY');
@a = py_call_function('__main__', 'bounce_array', [Foo->new]);
ok(@a == 1);

is((bounce_array([1, 2, 3]))[2], 3);

is((perl_list(Foo->new))[2], 3);

my @b = (0.1,0.2,0.3,0.4);
is((bounce_array(\@b))[0], 0.1);

map($b[$_]+$b[$_], 0..$#b);
is((bounce_array(\@b))[1], 0.2);

package Foo;

sub new {
    return bless {};
}

sub list {
    return (1, 2, 3);
}
