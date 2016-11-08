#
# Returning Python dictionaries with 'unicode' keys might lead to memory leaks.
# This test creates a memory leak situation and verifies that RSS usage is normal.
#

use strict;
use warnings;

use Test::More tests => 2;
use Test::Deep;

use Proc::ProcessTable;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';

def python_dict_with_unicode_key():
    return {
        u'abcdefghijklmno': 1,
        u'pqrstuvwxyz': 2,
    }

END

sub get_rss_memory {
    my $pt = Proc::ProcessTable->new;
    my %info = map { $_->pid => $_ } @{ $pt->table };
    return $info{ $$ }->rss;
}

my $iterations = 3_000_000;

my $rss_before_iterations = get_rss_memory();
# print STDERR "RSS (KB) before python_dict_with_unicode_key(): $rss_before_iterations\n";

my $dict;
for (my $x = 0; $x < $iterations; ++$x) {
    $dict = python_dict_with_unicode_key();
}
my $rss_after_iterations = get_rss_memory();
# print STDERR "RSS (KB) after python_dict_with_unicode_key(): $rss_after_iterations\n";

ok( $rss_after_iterations - $rss_before_iterations < 100 * 1024, "RSS takes up less than 100 MB" );
cmp_deeply( $dict, { 'abcdefghijklmno' => 1, 'pqrstuvwxyz' => 2 } );
