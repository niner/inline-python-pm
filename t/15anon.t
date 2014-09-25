use Test;
BEGIN { plan tests => 2 }
use Inline Config => DIRECTORY => './blib_test';
use Inline PYTHON;

$o = Foo();
ok(defined $o);
ok($o->method("Python"), 42);

__END__
__PYTHON__

def Foo():
    class Bar:
        def __init__(self): pass
        def method(self, lang):
            print("Note: lang=%s" % lang)
            return 42
    return Bar()
