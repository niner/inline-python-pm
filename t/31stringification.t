use strict;
use warnings;

use Inline Config => DIRECTORY => './blib_test';
use Test::More tests => 2;

use Inline Python => <<END;

class Stringify:
    def __init__(self, foo):
        self.foo = foo
    def __str__(self):
        return self.foo

class NoString:
    def __init__(self, foo):
        self.foo = foo

END

my $stringify = Stringify->new('foo');
my $nostring = NoString->new('foo');

is("$stringify", 'foo');
like("$nostring", qr/NoString/);
