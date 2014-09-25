use Test;

BEGIN { plan tests => 9 }

package Fart::Fiddle;

sub new {
    my $class = shift || __PACKAGE__;
    print "Creating new ", __PACKAGE__, ": $class\n";
    return bless {}, $class;
}

sub foof {
    my $o = shift;
    print Data::Dumper::Dumper("Fiddle->foof(", @_, ") called!");
    main::ok(1);
}

package main;

use Data::Dumper;
use Inline Config => DIRECTORY => './blib_test';
use Inline::Python qw(py_eval);
use Inline Python => <<END;

class A:
    def __init__(self):
        self.data = {}
    def foof(self):
        print("Hello, back in Python...")

def gen(): return A()

END

ok(1);
py_eval("print(dir(perl))");
ok(1);
my $o = new A;
ok(1);
undef $o;
print "It's gone now...\n";

py_eval(<<END);
o = perl.Fart.Fiddle.new()
if o: perl.ok(1)
print(o)
o.foof({'neil': 1}, ['laura', 1], 12)
perl.ok(1)
perl.eval('print qq{Hello. This is Neil\\n}')
perl.ok(1)
perl.use('CGI')
perl.ok(1)
END

ok(defined $::{'CGI::'});
