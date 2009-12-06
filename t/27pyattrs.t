use Test::More tests => 6;

use Inline Config => DIRECTORY => './blib_test';

use Inline Python => <<'END';

class Foo:
    def __init__(self):
        self.set_foo()

    def get_foo(self):
        return self.foo

    def set_foo(self):
        self.foo = 'foo'

    def __getattr__(self, attr):
        if attr == 'bar':
            return 'bar'

END

my $foo = Foo->new();

is($foo->get_foo, 'foo', 'constructor worked');
is($foo->{foo}, 'foo', 'get attribute');

$foo->{foo} = 'bar';

is($foo->get_foo, 'bar', 'set attribute');
is($foo->{foo}, 'bar', 'get attribute after set attribute');

$foo->set_foo;
is($foo->{foo}, 'foo', 'get attribute after Python object changes');

is($foo->{bar}, 'bar', '__getattr__ method also works');
