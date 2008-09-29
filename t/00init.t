BEGIN {
   print "1..1\n";
}

use File::Path;

rmtree("./blib_test");
mkdir("./blib_test", 0777) or print "not ok 1\n" && exit;

print "ok 1\n";
