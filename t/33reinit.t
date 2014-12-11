use Test;
BEGIN { plan tests => 10 }

use Inline::Python qw();

for (1 .. 10) {
    Inline::Python::py_finalize;
    Inline::Python::py_initialize;
    ok(Inline::Python::py_eval("True", 0));
}
