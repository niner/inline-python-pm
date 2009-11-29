use Test;
BEGIN { plan tests => 6 }
use Inline Config => DIRECTORY => './blib_test';
use Inline::Python qw(py_call_function);
use Inline Python => <<'END';

def return_empty_array():
    return []

def return_onesized_array():
    return [1]

def bounce_array(a):
    return a

END

ok(ref scalar py_call_function('__main__', 'return_empty_array') eq 'ARRAY');
my @a = py_call_function('__main__', 'return_empty_array');
ok(@a == 0);

ok(ref scalar py_call_function('__main__', 'return_onesized_array') eq 'ARRAY');
@a = py_call_function('__main__', 'return_onesized_array');
ok(@a == 1);

ok(ref scalar py_call_function('__main__', 'bounce_array', [Foo->new]) eq 'ARRAY');
@a = py_call_function('__main__', 'bounce_array', [Foo->new]);
ok(@a == 1);

package Foo;

sub new {
    return bless {};
}
