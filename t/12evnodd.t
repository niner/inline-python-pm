use Test;
BEGIN { plan tests => 150 }
use Inline Config => DIRECTORY => './blib_test';
use Inline Python;

sub even {
    my $n = shift;
    return $n == 0 ? 1 : odd($n-1);
}

for (my $i=0; $i<75; $i++) {
    my $r = $i % 2;
    ok(odd($i), $r);
    ok(even($i), 1 - $r);
}

__END__
__Python__

# Need to explicitly import Perl function
even = perl.even

def odd(n):
    if n == 0: return 0
    return even(n-1)
