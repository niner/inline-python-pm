use Test;
BEGIN { plan tests => 133 }
use Config;
use Inline Config => DIRECTORY => './blib_test';
use Inline Python;

my $ovfl = $Config{intsize} <= 4 ? "Integer too large for architecture" : 0;

sub fact {
    my $num = shift;
    my $skip = shift || 0;
    return fact_help($num, 1, $skip);
}

# OK on 32- and 64-bit machines.
ok(fact(1), 1);
ok(fact(2), 2);
ok(fact(3), 6);
ok(fact(4), 24);
ok(fact(5), 120);
ok(fact(6), 720);
ok(fact(7), 5040);
ok(fact(8), 40320);
ok(fact(9), 362880);
ok(fact(10), 3628800);

# These ones tip the scales on 32-bit machines.
skip($ovfl, fact(11, $ovfl), 39916800);
skip($ovfl, fact(12, $ovfl), 479001600);
skip($ovfl, fact(13, $ovfl), 6227020800);
skip($ovfl, fact(14, $ovfl), 87178291200);

__END__
__Python__

def fact_help(num, sofar, skip):
    perl.skip(skip, "skip")
    if num == 0: return sofar
    if skip: return fact_help(num-1, num, skip) # don't multiply
    return fact_help(num-1, num*sofar, skip)
