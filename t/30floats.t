use strict;
use warnings;

use Inline Config => DIRECTORY => './blib_test';
use Test::More tests => 7;
use Test::Number::Delta;
use POSIX qw(setlocale LC_NUMERIC);

use Inline Python => <<END;
def pyprint(*args):
    return str(args)

def give_float():
    return 1.2

def is_float(x):
    return isinstance(x, float)

END

delta_ok(parse_py_list(pyprint(0.1 + 0.1)), 0.2);
my @a = (0.1, 0.2, 0.3, 0.4);
delta_ok(parse_py_array(pyprint(\@a)), \@a);

@a = map($a[$_] + $a[$_], 0 .. $#a);
delta_ok(parse_py_array(pyprint(\@a)), \@a);

@a = map($_ * 1.0, @a);
delta_ok(parse_py_array(pyprint(\@a)), \@a);

# test if float conversion works despite localized number format
setlocale LC_NUMERIC, "de_DE.UTF-8";
delta_ok(parse_py_list(pyprint(0.25)), 0.25);

ok(is_float(0.1), "Perl float arrives as float in Python");
ok(is_float(give_float()), "Python float arrives as float in Perl (and can be passed through)");

sub parse_py_list {
    my ($str) = @_;
    my ($num) = $str =~ /\((\d+\.\d+),\)/;
    return $num;
}

sub parse_py_array {
    my ($str) = @_;
    my @num;
    push @num, $1 while $str =~ /(\d+\.\d+)/g;
    return \@num;
}
