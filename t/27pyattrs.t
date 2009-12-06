use Test::More tests => 5;

use Inline Config => DIRECTORY => './blib_test';

use Inline Python => <<'END';

class Foo:
    def __init__(self):
        self.set_foo()

    def get_foo(self):
        return self.foo

    def set_foo(self):
        self.foo = 'foo'

END

my $foo = Foo->new();

is($foo->get_foo, 'foo');
is($foo->{foo}, 'foo');

$foo->{foo} = 'bar';

is($foo->get_foo, 'bar');
is($foo->{foo}, 'bar');

$foo->set_foo;
is($foo->{foo}, 'foo');
