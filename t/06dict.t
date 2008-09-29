use Test;
BEGIN { plan tests => 2 }
use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';

class Foo:
    def __init__(self):
        print "new Foo object being created"
        self.data = {}
    def get_data(self): return self.data
    def set_data(self,dat): 
        self.data = dat

END

my $obj = new Foo;
ok(not keys %{$obj->get_data()});

$obj->set_data({string => 'hello',
		number => 0.7574,
		array => [1, 2, 3],
	       });
ok($obj->get_data()->{string}, "hello");
