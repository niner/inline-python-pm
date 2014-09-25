use Inline Config => DIRECTORY => './blib_test';

BEGIN {
   print "1..1\n";
}

use Inline Python => <<'END';
import sys
if sys.version_info[0] < 3:
    def JAxH(x): return "Just Another %s Hacker" % x
else:
    def JAxH(x): return "Just Another %s Hacker" % x.decode('utf-8')
END

print "not " unless JAxH('Inline') eq "Just Another Inline Hacker";
print "ok 1\n";

