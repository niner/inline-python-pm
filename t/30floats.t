use strict;
use warnings;

use Inline Config => DIRECTORY => './blib_test';
use Test::More tests => 4;

use Inline Python => <<END;
def pyprint(*args):
    return str(args)

END

is(pyprint(0.1 + 0.1), '(0.20000000000000001,)');
my @a = (0.1,0.2,0.3,0.4);
is(pyprint(\@a), '([0.10000000000000001, 0.20000000000000001, 0.29999999999999999, 0.40000000000000002],)'); # Correct output

map($a[$_]+$a[$_], 0..$#a);
is(pyprint(\@a), '([0.10000000000000001, 0.20000000000000001, 0.29999999999999999, 0.40000000000000002],)'); # Incorrect output (all zeros)

@a = map($_*1.0, @a);
is(pyprint(\@a), '([0.10000000000000001, 0.20000000000000001, 0.29999999999999999, 0.40000000000000002],)'); # Correct output
