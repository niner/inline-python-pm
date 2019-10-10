use strict;
use warnings;
use utf8;

use Test::More tests => 2;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';
def add_x(string):
    return 'x' + string
END

my $str_utf8  = 'abć';
my $str_ascii = 'abc';

is add_x($str_utf8),  "x$str_utf8",  'string op on unicode string';
is add_x($str_ascii), "x$str_ascii", 'string op on ascii string';
