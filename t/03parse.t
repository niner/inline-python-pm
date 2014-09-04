use Test::More;

eval { require Parse::RecDescent; };
plan skip_all => "Test requires Parse::RecDescent: $@" if $@;
plan tests => 5;

sub ok_is { # Python does not allow "is" as method name
    &is;
}

use Inline::Python qw(py_eval);

py_eval(<<'END');
perl.ok_is(1, 1) # Well, we got this far...
perl.use("Data::Dumper")
perl.use("Parse::RecDescent")
perl.eval('print "Hello\n"')
print perl.Data.Dumper.Dumper({'neil': 'happy', 'others': 'sad'})
o = perl.Parse.RecDescent.new("Parse::RecDescent","dumb: 'd' 'u' 'm' 'b'")
perl.ok_is(o.dumb("dumb"), 'b')
perl.ok_is(o.dumb("dork"), None)

END

$o = Parse::RecDescent->new("dumb: 'd' 'u' 'm' 'b'");
is($o->dumb("dork"), undef);
is($o->dumb("dumb"), 'b');
