use Test::More tests => 6;

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

END

eval {
    error();
};
ok(1, 'Survived Python exception');
is($@, "exceptions.Exception: Error!\n", 'Exception found');

eval {
    empty_error();
};
is($@, "exceptions.Exception: \n", 'Exception found');

eval {
    name_error();
};
is($@, "exceptions.NameError: global name 'foo' is not defined\n", 'NameError found');

my $foo = Foo->new;
eval {
    $foo->error;
};
is($@, "exceptions.Exception: Error!\n", 'Exception found');

eval {
    thrower()->();
};
is($@, "exceptions.NameError: global name 'foo' is not defined\n", 'NameError found');
