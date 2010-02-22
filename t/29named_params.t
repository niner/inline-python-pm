use Test::More tests => 2;
use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';

use Inline Python => <<END;

def call_method(obj):
    perl.ok(obj.meth(0, 1, 2) == 2)
    perl.ok(obj.meth(0, b=2) == 2)

END

call_method(bless {}, 'Named');

package Named;

sub meth {
    my ($self, $x, $a, $b) = @_;

    if (ref $x and ref $x eq 'ARRAY' and ref $a and ref $a eq 'HASH') {
        return $a->{b};
    }
    else {
        return $b;
    }
}
