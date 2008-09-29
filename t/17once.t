use Test;
BEGIN { plan tests => 1 }

# Need to define the subs BEFORE the 'use Inline' statement, since the
# Python code will run them.
my $build_stage;
my $load_stage;
sub login {
    $build_stage ? $load_stage++ : $build_stage++;
}

use Inline Python;

die "Error -- code was run both in build and load stages!"
  if $build_stage && $load_stage;

ok(1);

__END__
__Python__
# Put some code in the 'main' section -- it will be run during
# Inline's "build" phase, but NOT run again in the "load" phase.

perl.login();
