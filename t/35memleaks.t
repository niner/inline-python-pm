#
# Try to detect memory leaks in a rather rudimentary way: by comparing memory
# usage before and after calling potentially-leaking code a bunch of times.
#

use strict;
use warnings;

use Test::More;
eval { require Proc::ProcessTable; require Test::Deep; };
plan skip_all => "Test requires Proc::ProcessTable and Test::Deep: $@" if $@;
plan tests => 4;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';

def python_dict_with_unicode_key():
    return {
        u'abcdefghijklmno': 1,
        u'pqrstuvwxyz': 2,
    }

def python_throw_exception():
    raise Exception('abcdefghijklmnopqrstuvwxyz')

END

sub get_rss_memory {
    my $pt = Proc::ProcessTable->new;
    my %info = map { $_->pid => $_ } @{ $pt->table };
    my $rss = $info{ $$ }->rss;
    if ($^O eq 'darwin') {
    	# RSS is reported in kilobytes instead of bytes on OS X
    	$rss *= 1024;
    }
    return $rss;
}

{
    # Inline::Python was leaking memory when returning dictionaries with 'unicode' keys

    my $rss_before_iterations = get_rss_memory();
    # print STDERR "RSS before python_dict_with_unicode_key(): $rss_before_iterations\n";

    my $dict;
    for (my $x = 0; $x < 3_000_000; ++$x) {
        $dict = python_dict_with_unicode_key();
    }
    my $rss_after_iterations = get_rss_memory();
    # print STDERR "RSS after python_dict_with_unicode_key(): $rss_after_iterations\n";

    ok( $rss_after_iterations - $rss_before_iterations < 100 * 1024 * 1024, "RSS takes up less than 100 MB" );
    Test::Deep::cmp_deeply( $dict, { 'abcdefghijklmno' => 1, 'pqrstuvwxyz' => 2 } );    
}

{
    # Make sure Inline::Python doesn't leak memory when handling exception tracebacks

    my $rss_before_iterations = get_rss_memory();
    # print STDERR "RSS before python_dict_with_unicode_key(): $rss_before_iterations\n";

    my $dict;
    for (my $x = 0; $x < 1_000_000; ++$x) {
        eval {
            python_throw_exception();
        };
    }
    my $rss_after_iterations = get_rss_memory();
    # print STDERR "RSS after python_dict_with_unicode_key(): $rss_after_iterations\n";

    ok( $rss_after_iterations - $rss_before_iterations < 100 * 1024 * 1024, "RSS takes up less than 100 MB" );
    eval { python_throw_exception(); };
    like( $@, qr/abcdefghijklmnopqrstuvwxyz/ );
}
