package Items;

sub new {
    return bless {
        test => 'Items test!',
    };
}

sub __getitem__ {
    my ($self, $item) = @_;
    return unless exists $self->{$item};
    return $self->{$item};
}

package NoItems;

sub new {
    return bless {
        test => 'NoItems test!',
    };
}

package main;

use Test;
use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';

BEGIN { plan tests => 4 }

use Inline::Python qw(py_eval py_call_function);

py_eval(<<'END');

def test_items(foo, s):
    perl.ok(foo['test'] == s)
    perl.ok(foo['test'] == s)

def test_noitems(bar):
    try:
        bar['test']
    except TypeError:
        return 1
    return 0

END

ok(py_call_function("__main__", "test_items", Items->new, 'Items test!'), undef);
ok(py_call_function("__main__", "test_noitems", NoItems->new) == 1);
