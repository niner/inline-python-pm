use Test;

BEGIN { plan tests => 9 }

use Data::Dumper;
use Inline::Python qw(py_eval
		      py_new_object
		      py_call_method
		      );

py_eval <<END;

class Foo:
    def __init__(self):
        print "New foo being created!"
	self.data = {}
    def watchit(self):
        print "Watching it, sir!"
	print self.data
    def put(self, key, value):
	self.data[key] = value
    def get(self, key):
	try:
	    return self.data[key]
	except KeyError:
	    return None

END

ok(1);

#============================================================================
# We can use the constructor for Inline::Python::Object to create a new 
# instance. It has an AUTOLOAD which handles the method calls for us.
#============================================================================
my $o = Inline::Python::Object->new('__main__', 'Foo');
ok($o->put("neil", { is => 'cool', was => 'stupid' }), undef);
ok($o->get("foobar"), undef);
my $r = $o->get("neil");
ok($r->{is}, 'cool');
ok($r->{was}, 'stupid');

#============================================================================
# Or, we can use the direct-version: py_new_object takes a Perl package to
# bless the result into, then the same arguments as Inline::Python::Object.
#============================================================================
$o = py_new_object('main::Quack', '__main__', 'Foo');
ok(py_call_method($o, 'put', "neil", { is => 'cool', was => 'stupid' }), undef);
ok(py_call_method($o, 'get', "foobar"), undef);
$r = py_call_method($o, 'get', "neil");
ok($r->{is}, 'cool');
ok($r->{was}, 'stupid');
