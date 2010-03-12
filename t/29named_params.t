use Test::More tests => 5;
use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';

our $fiddles = 0;

use Inline Python => <<END;

def call_method(obj, param):
    perl.ok(obj.meth(0, 1, 2) == 2)
    perl.ok(obj.meth(0, b=2) == 2)
    perl.ok(obj.meth(param, b=2) == 2)

END

{
    my $fiddle = Fart::Fiddle->new;
    call_method(Named->new, $fiddle);
    is($fiddle->foo, 'foo');
}
is($fiddles, 0, 'objects got destroyed');

package Named;

sub new {
    return bless {};
}

sub meth {
    my ($self, $x, $a, $b) = @_;

    if (ref $x and ref $x eq 'ARRAY' and ref $a and ref $a eq 'HASH') {
        my $params = $a;
        foreach (qw( x a b )) {
            last unless @$x;
            $params->{$_} = shift @$x;
        }
        return $a->{b};
    }
    else {
        return $b;
    }
}

package Fart::Fiddle;

sub new {
    $::fiddles++;
    return bless {};
}

sub foo {
    return 'foo';
}

sub DESTROY {
    my $self = shift;
    $::fiddles--;
}
