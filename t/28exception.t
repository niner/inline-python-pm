use strict;
use warnings;
use Test::More tests => 12;

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

class CustomException(Exception):
    pass

def custom_exception():
    raise CustomException()

def zero_division_error():
    return 1 / 0

def unicode_decode_error():
    return b"\\xc3\\x28".decode('utf-8')

END

eval {
    error();
};
ok(1, 'Survived Python exception');
like($@, qr/line \d+, in error\s+Exception: Error!/, 'Exception found');

eval {
    empty_error();
};
like($@, qr/line \d+, in empty_error\s+Exception/, 'Exception found');

eval {
    name_error();
};
like($@, qr/line \d+, in name_error\s+NameError:( global)? name 'foo' is not defined/, 'NameError found');

my $foo = Foo->new;
eval {
    $foo->error;
};
like($@, qr/line \d+, in error\s+Exception: Error!/, 'Exception found');

eval {
    thrower()->();
};
like($@, qr/line \d+, in <lambda>\s+NameError:( global)? name 'foo' is not defined/, 'Exception found');

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

eval {
    custom_exception();
};
like($@, qr/CustomException/);

eval {
    zero_division_error();
};
like($@, qr/ZeroDivisionError/);

eval {
    unicode_decode_error();
};
like($@, qr/UnicodeDecodeError/);
