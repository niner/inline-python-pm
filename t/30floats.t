use strict;
use warnings;

use Inline Config => DIRECTORY => './blib_test';
use Test::More tests => 5;
use POSIX qw(setlocale LC_NUMERIC);

use Inline Python => <<END;
def pyprint(*args):
    return str(args)

END

like(pyprint(0.1 + 0.1), qr/\(0\.2(0000000000000001)?,\)/);
my @a = (0.1,0.2,0.3,0.4);
like(pyprint(\@a), qr/\(\[0\.1(0000000000000001)?, 0\.2(0000000000000001)?, 0\.(29999999999999999|3), 0\.4(0000000000000002)?\],\)/); # Correct output

map($a[$_]+$a[$_], 0..$#a);
like(pyprint(\@a), qr/\(\[0\.1(0000000000000001)?, 0\.2(0000000000000001)?, 0\.(29999999999999999|3), 0\.4(0000000000000002)?\],\)/); # Incorrect output (all zeros)

@a = map($_*1.0, @a);
like(pyprint(\@a), qr/\(\[0\.1(0000000000000001)?, 0\.2(0000000000000001)?, 0\.(29999999999999999|3), 0\.4(0000000000000002)?\],\)/); # Correct output

# test if float conversion works despite localized number format
setlocale LC_NUMERIC, "de_DE.UTF-8";
is(pyprint(0.25), '(0.25,)');
