use Test;
BEGIN { plan tests => 4 }

use Inline::Python qw(py_eval py_call_function);

py_eval(<<END);
class Bar:
    def __init__(self): 
        self.data = {}
        print "new Bar being created!"
    def put(self, key, val): self.data[key] = val
    def get(self, key): 
        try: return self.data[key]
        except KeyError: return None

def Foo(): return 42
END

ok(not defined py_eval("Foo()"));
ok(    defined py_eval("Foo()", 0));
ok(not defined py_eval("Foo()", 1));
ok(not defined py_eval("Foo()", 2));
