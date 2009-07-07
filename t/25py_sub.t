use Test;
use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';

BEGIN { plan tests => 2 }

use Inline Python => <<END;

def get_sub():
    return lambda: 'hello Python'

def get_sub_with_arg():
    return lambda x: x

END

my $sub = get_sub();
ok($sub->(), 'hello Python');
$sub = get_sub_with_arg();
ok($sub->('hello Python'), 'hello Python');
