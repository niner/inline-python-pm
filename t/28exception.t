use strict;
use warnings;
use Test::More tests => 9;

use Inline Config => DIRECTORY => './blib_test';

use Inline Python => <<END;

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
    except Exception, e:
        return e.message

def pass_through_perl_exception(failer):
    failer()

END

eval {
    error();
};
ok(1, 'Survived Python exception');
like($@, qr/Error! at line 3/, 'Exception found');

eval {
    empty_error();
};
like($@, qr/Exception:  at line 6/, 'Exception found');

eval {
    name_error();
};
like($@, qr/name 'foo' is not defined at line 9/, 'NameError found');

my $foo = Foo->new;
eval {
    $foo->error;
};
like($@, qr/Exception: Error! at line 13/, 'Exception found');

eval {
    thrower()->();
};
like($@, qr/name 'foo' is not defined at line 16/, 'Exception found');

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
