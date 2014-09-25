use Test::Simple tests => 2;
use strict;
use warnings;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';
def debug(x):
    return str(x)

def PyVersion(): import sys; return sys.version_info[0]

END

my @a = ('foo' , 'bar', 'baz');
delete $a[1];

ok(debug(undef) eq 'None');
ok(debug(\@a) eq "['foo', None, 'baz']") if PyVersion() == 2;
ok(debug(\@a) eq "[b'foo', None, b'baz']") if PyVersion() == 3;
