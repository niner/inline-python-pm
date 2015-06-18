use strict;
use warnings;
use Test::More tests => 9;

use Inline Config => DIRECTORY => './blib_test';

use Inline Python => <<END;

import sys

def error():
    raise Exception('Error!')

def empty_error():
    raise Exception()

def name_error():
    return foo

class Foo:
    def error(self):
        raise Exception('Error!')

def thrower():
    return lambda: foo

def catch_perl_exception(failer):
    try:
        failer()
    except Exception:
        exc_type, e, exc_traceback = sys.exc_info()
        return str(e)

def pass_through_perl_exception(failer):
    failer()

END

eval {
    error();
};
ok(1, 'Survived Python exception');
like($@, qr/Error! at line 5/, 'Exception found');

eval {
    empty_error();
};
like($@, qr/Exception:  at line 8/, 'Exception found');

eval {
    name_error();
};
like($@, qr/name 'foo' is not defined at line 11/, 'NameError found');

my $foo = Foo->new;
eval {
    $foo->error;
};
like($@, qr/Exception: Error! at line 15/, 'Exception found');

eval {
    thrower()->();
};
like($@, qr/name 'foo' is not defined at line 18/, 'Exception found');

my $exception = catch_perl_exception(sub { die "fail!"; });
like($exception, qr/fail!/);

eval {
    pass_through_perl_exception(sub { die "fail!"; });
};
like($@, qr/fail!/);

my $foo_exception = bless {}, 'FooException';

eval {
    pass_through_perl_exception(sub { die $foo_exception; });
};
is(ref $@, 'FooException');
