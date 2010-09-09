use Test::More tests => 14;

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
        raise AttributeError(attr)

class KillMe:
    def __getattr__(self, attr):
        raise KeyError(attr)

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

is(Inline::Python::py_has_attr($foo, 'non_existing'), 0, 'py_has_attr returns 0 for non-existing attribute');
is(Inline::Python::py_has_attr($foo, 'foo'), 1, 'py_has_attr returns 1 for existing attribute');

ok(not(eval { $foo->{non_existing} }), 'Surviving accessing a non existent attribute');

ok(Inline::Python::py_get_attr($foo, 'get_foo'), 'Can access methods via py_get_attr');
ok($foo->{get_foo}, 'Can access methods as attributes');
is($foo->{get_foo}->(), 'foo', 'Returned method works');

my $killer = KillMe->new();
ok(not(eval { $killer->{foo} }), 'survived KeyError in __getattr__');
is($@, "exceptions.KeyError: 'foo' at line 19\n", 'Got the KeyError');
