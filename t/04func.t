use Test;
use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';

BEGIN { plan tests => 3 }

use Inline::Python qw(py_eval py_call_function);

ok(py_eval("print('Hello from Python!')"), undef);

py_eval(<<'END');

class Foo:
	def __init__(self):
		print("Foo() created!")
	def apple(self): 
		print("Doing an apple!")

def funky(a):
	print(a)

END

ok(py_call_function("__main__","funky",{neil=>'happy'}), undef);

my $o = py_call_function("__main__","Foo");
ok($o->apple, undef);
print Dumper $o;
print Dumper $o->apple;
