use Test::More;
use strict;
use warnings;
use utf8;

eval { require 5.008; };
plan skip_all => 'Perl 5.8 required for UTF8 tests' if $@;
plan tests => 5;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';
from types import StringType, UnicodeType

def u_string():
    return u"Hello"

def string():
    return "Hello"

def is_unicode(a):
    return isinstance(a, UnicodeType)

def unicode_string():
    return u'a'

def pass_through(a):
    return a

END

ok(string() eq 'Hello');
ok(u_string() eq 'Hello');

ok(is_unicode('ö'), 'perl utf8 -> python unicode');
ok(utf8::is_utf8(unicode_string()), 'python unicode -> perl utf8');

ok(pass_through('ö') eq 'ö', 'utf8ness retained');
