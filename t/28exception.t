use Test::More tests => 2;

use Inline Config => DIRECTORY => './blib_test';

use Inline Python => <<END;

def error():
    raise Exception('Error!')

END

open my $old_err, '>&', \*STDERR;
close STDERR;

eval {
    error();
};

open STDERR, '>&', $old_err;

ok(1, 'Survived Python exception');

ok($@, 'Exception found');
