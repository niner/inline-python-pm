use Test::More;
use strict;
use warnings;
use utf8;

eval { require 5.008; };
plan skip_all => 'Perl 5.8 required for UTF8 tests' if $@;
plan tests => 5;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => <<'END';

def PyVersion(): import sys; return sys.version_info[0]

def string():
    return "Hello"

def pass_through(a):
    return a

if PyVersion() < 3:
    from types import StringType, UnicodeType

    def u_string():
        return eval("u'Hello'")

    def is_unicode(a):
        return isinstance(a, UnicodeType)

    def unicode_string():
        return eval("u'a'")
else:
    def b_string():
        return eval("b'Hello'")

END

ok(string() eq 'Hello');
ok(pass_through('ö') eq 'ö', 'utf8ness retained');

if(PyVersion() < 3) {
	ok(u_string() eq 'Hello');

	ok(is_unicode('ö'), 'perl utf8 -> python unicode');
	ok(utf8::is_utf8(unicode_string()), 'python unicode -> perl utf8');

}
else {
	ok(b_string() eq 'Hello');

	ok(!utf8::is_utf8(b_string()), 'python bytes -> not perl utf8');
	ok(utf8::is_utf8(string()), 'python unicode -> perl utf8');
}
