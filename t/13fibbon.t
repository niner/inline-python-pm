use Test;
BEGIN { plan tests => 277 }
use Inline::Python qw(py_eval py_bind_func);

# Bind __main__.fibb to main::fibb
py_bind_func("main::fibb", "__main__", "fibb");

# And fill in the gap:
py_eval(<<END);
def fibb(n):
    perl.ok(1)
    if n == 0: return (1, 0)
    if n == 1: return (1, 1)
    a, b = fibb(n - 1)
    return (a+b, a)
END

@fibb = qw(0
	   1
	   1
	   2
	   3
	   5
	   8
	   13
	   21
	   34
	   55
	   89
	   144
	   233
	   377
	   610
	   987
	   1597
	   2584
	   4181
	   6765
	   10946
	   17711
	  );

for (my $i=0; $i<@fibb; $i++) {
    ok((fibb($i))[1], $fibb[$i]);
}
