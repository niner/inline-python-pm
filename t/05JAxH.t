use Inline Config => DIRECTORY => './blib_test';

BEGIN {
   print "1..1\n";
}

use Inline Python => <<'END';
def JAxH(x): return "Just Another %s Hacker" % x
END

print "not " unless JAxH('Inline') eq "Just Another Inline Hacker";
print "ok 1\n";

