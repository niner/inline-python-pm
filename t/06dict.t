use strict;
use warnings;
use utf8;

use Test::More tests => 6;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';
# coding=utf-8

class Foo:
    def __init__(self):
        print "new Foo object being created"
        self.data = {}
    def get_data(self): return self.data
    def set_data(self,dat):
        self.data = dat
        self.data[u'ü'] = u'ü'

def get_dict():
    return {u'föö': 'bar'}
def access_dict(test_dict):
    return test_dict[u'föö']
END

my $obj = new Foo;
ok(not keys %{$obj->get_data()});

$obj->set_data({string => 'hello',
		number => 0.7574,
		array => [1, 2, 3],
                ütf8 => 'töst',
	       });
is($obj->get_data()->{string}, 'hello');
is($obj->get_data()->{ütf8}, 'töst');
is($obj->get_data()->{ü}, 'ü');

is(access_dict({föö => 'bar'}), 'bar');
my $dict = get_dict();
is(access_dict($dict), 'bar');
