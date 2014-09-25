package Attrs;

sub new {
    return bless {
        test => 'Attrs test!',
    };
}

sub __getattr__ {
    my ($self, $attr) = @_;
    return unless exists $self->{$attr};
    return $self->{$attr};
}

package NoAttrs;

sub new {
    return bless {
        test => 'NoAttrs test!',
    };
}

package main;

use Test;
use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';

BEGIN { plan tests => 6 }

use Inline::Python qw(py_eval py_call_function);

py_eval(<<'END');

def test_attrs(foo, s):
    perl.ok(hasattr(foo, 'test'))
    perl.ok(not hasattr(foo, 'foo'))
    perl.ok(foo.test == s)
    perl.ok(foo.__getattr__('test') == s)

def test_noattrs(bar):
    try:
        perl.warn(bar.test)
    except AttributeError:
        return 1
    return 0

END

ok(py_call_function("__main__", "test_attrs", Attrs->new, 'Attrs test!'), undef);
ok(py_call_function("__main__", "test_noattrs", NoAttrs->new) == 1);
