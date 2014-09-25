use strict;
use warnings;
use utf8;

use Test::More tests => 6;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';
# coding=utf-8

def PyVersion(): import sys; return sys.version_info[0]

class Foo:
    def __init__(self):
        print("new Foo object being created")
        self.data = {}
    def get_data(self): return self.data
    def set_data(self,dat):
        self.data = dat
        if PyVersion() == 3:
            self.data['ü'] = 'ü'
        else:
            # u'ü' is not a valid syntax in Py3.1
            s = '\xc3\xbc'.decode('utf-8') 
            self.data[s] = s

def get_dict():
    if PyVersion() == 3:
        return {'föö': 'bar'}
    else:
        # u'föö' is not a valid syntax in Py3.1
        return {'f\xc3\xb6\xc3\xb6'.decode('utf-8'): 'bar'}

def access_dict(test_dict):
    if PyVersion() == 3:
        return test_dict['föö']
    else:
        # u'föö' is not a valid syntax in Py3.1
        return test_dict['f\xc3\xb6\xc3\xb6'.decode('utf-8')]
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
