use Test::More;
use Inline Python => <<PYTHON;
import os
os.environ["TEST_VARIABLE"] = "BOB"
PYTHON
ok(1, 'survived Python setting an env variable');
done_testing;
# might explode during cleanup but the test harness will catch that
