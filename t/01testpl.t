use Test;

BEGIN { plan tests => 8 }

use Inline Python => Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';

def check_for_sub(sub):
    try: 
        f = getattr(perl,sub)
        if type(f).__name__ == "_perl_sub": 
            print("Sub %s exists!" % sub)
            return 1
        else:
            print("%s is not a sub!" % sub)
            return 0
    except AttributeError:
        print("Sub %s not found!" % sub)
        return 0

def get_sub(sub):
    if check_for_sub(sub): return getattr(perl,sub)
    else: raise AttributeError("No such sub")

END

ok(1); #loaded

sub f { print "Hello from Perl\n"; ok(1); }

use Inline::Python qw(py_eval);

py_eval(<<END);
perl.use("Data::Dumper")
perl.f()
print(perl.Dumper({'neil': 0, 'laura': 1})), # suppress extra \n
perl.ok(1)
print(perl.CORE)
perl.ok(1)
print(dir(perl))
perl.ok(1)
t = get_sub('f')
t.flags = t.G_EVAL | t.G_KEEPERR
t()

perl.py_eval("print('Wow. Now that is weird')")
perl.ok(1)

END

ok(1)
