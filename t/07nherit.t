use Test;
BEGIN { plan tests => 5 }
use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';

class Daddy:
    def __init__(self):
        print("Who's your daddy?")
        self.fish = []
    def push(self,dat):
        print("Daddy.push(%s)" % dat)
        return self.fish.append(dat)
    def pop(self):
        print("Daddy.pop()")
        return self.fish.pop()

class Mommy:
    def __init__(self, s):
        print("Who's your mommy?")
        self.jello = s
    def add(self,data):
        self.jello = self.jello + data
        return self.jello
    def takeaway(self,data):
        self.jello = self.jello[0:-len(data)]
        return self.jello

class Foo(Daddy,Mommy):
    def __init__(self, s):
        print("new Foo object being created")
        self.data = {}
        Daddy.__init__(self)
        Mommy.__init__(self, s)
    def get_data(self): return self.data
    def set_data(self,dat): 
        self.data = dat

END

my $obj = new Foo("hello");
ok(not keys %{$obj->get_data()});

$obj->set_data({string => 'hello',
		number => 0.7574,
		array => [1, 2, 3],
	       });
ok($obj->get_data()->{string}, "hello");

$obj->push(12);
ok($obj->pop(), 12);
ok($obj->add("wink"), "hellowink");
ok($obj->takeaway("fiddle"), "hel");
