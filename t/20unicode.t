use Test::Simple tests => 2;
use strict;
use warnings;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';
def u_string():
    return u"Hello"

def string():
    return "Hello"
END

ok(string() eq 'Hello');
ok(u_string() eq 'Hello');

