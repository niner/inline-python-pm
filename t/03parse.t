use Test;

BEGIN { plan tests => 5 }

use Inline::Python qw(py_eval);

py_eval(<<'END');
perl.ok(1, 1) # Well, we got this far...
perl.use("Data::Dumper")
perl.use("Parse::RecDescent")
perl.eval('print "Hello\n"')
print perl.Data.Dumper.Dumper({'neil': 'happy', 'others': 'sad'})
o = perl.Parse.RecDescent.new("Parse::RecDescent","dumb: 'd' 'u' 'm' 'b'")
perl.ok(o.dumb("dumb"), 'b')
perl.ok(o.dumb("dork"), None)

END

$o = Parse::RecDescent->new("dumb: 'd' 'u' 'm' 'b'");
ok($o->dumb("dork"), undef);
ok($o->dumb("dumb"), 'b');
