use Test;
BEGIN { plan tests => 2 }
use Inline Config => DIRECTORY => './blib_test';
use Inline::Python qw(py_call_function);
use Inline Python => <<'END';

def return_array():
    return []

END

ok(ref scalar py_call_function('__main__', 'return_array') eq 'ARRAY');
my @a = py_call_function('__main__', 'return_array');
ok(@a == 0);
