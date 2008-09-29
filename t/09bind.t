use Test;

BEGIN { plan tests => 6 }

use Data::Dumper;
use Inline::Python qw(py_bind_class
		      py_bind_func
		      py_eval
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
# Import the Python class named "Foo" into the package called "main::Foo".
# Any extra arguments are made explicit methods -- without any extra args,
# all method calls go through AUTOLOAD, which is a performance hit.
#============================================================================
py_bind_class("main::Foo", "__main__", "Foo");
ok(1);

#============================================================================
# We can now use 'Foo' as if it were a Perl class.
#============================================================================
my $o = new Foo;
ok($o->put("neil", { is => 'cool', was => 'stupid' }), undef);
ok($o->get("foobar"), undef);
my $r = $o->get("neil");
ok($r->{is}, 'cool');
ok($r->{was}, 'stupid');
