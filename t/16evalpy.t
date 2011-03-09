use Test::More;
BEGIN { plan tests => 11 }
use Data::Dumper;
use Inline::Python qw(eval_python);

#============================================================================
# Pass code directly
#============================================================================
ok(not defined eval_python("1 + 2"));
ok(    defined eval_python("1 + 2", 0));
ok(not defined eval_python("1 + 2", 1));
ok(not defined eval_python("1 + 2", 2));

# Set up a function and class
eval_python(<<END);
def sum(*args):
    s = 0
    for i in args: s = s + i
    return s
class Bazz:
    def __init__(self): self.d = 1
    def scale(self, factor): self.d = self.d * factor
    def factor(self): return self.d
END

#============================================================================
# Evaluate a python function
#============================================================================
is(eval_python("__main__", "sum", 1, 2, 3), 6);
is(eval_python("sum(1, 2, 3)", 0), 6, "sum works");
ok($o = eval_python("__main__", "Bazz"));

#============================================================================
# Evaluate a python method
#============================================================================
is(eval_python($o, "scale", 10), undef);
is(eval_python($o, "factor"), 10);
is(eval_python($o, "scale", 0.05), undef);
is(eval_python($o, "factor"), 0.5);
