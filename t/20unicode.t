use Test::More;
use strict;
use warnings;
use utf8;
use Encode;

binmode(STDOUT, ':utf8');
binmode(STDERR, ':utf8');

eval { require 5.008; };
plan skip_all => 'Perl 5.8 required for UTF8 tests' if $@;
plan tests => 6;

use Inline Config => DIRECTORY => './blib_test';
use Inline Python => Encode::encode_utf8(<<'END');
# -*- coding: utf-8 -*-

def PyVersion(): import sys; return sys.version_info[0]

def string():
    return "Hello"

def pass_through(a):
    return a

def raise_exception(message):
    raise Exception(u"𝘜𝘯𝘪𝘤𝘰𝘥𝘦 𝘦𝘹𝘤𝘦𝘱𝘵𝘪𝘰𝘯: %s" % str(message))

if PyVersion() < 3:
    from types import StringType, UnicodeType

    def u_string():
        return eval("u'Hello'")

    def is_unicode(a):
        return isinstance(a, UnicodeType)

    def unicode_string():
        return eval("u'a'")

    def raise_exception(message):
        raise Exception(u"Unicode exception: %s" % message)
else:
    def b_string():
        return eval("b'Hello'")

    def raise_exception(message):
        raise Exception(u"𝘜𝘯𝘪𝘤𝘰𝘥𝘦 𝘦𝘹𝘤𝘦𝘱𝘵𝘪𝘰𝘯: %s" % message)
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

eval {
    raise_exception('𝘜𝘯𝘪𝘤𝘰𝘥𝘦 𝘢𝘳𝘨𝘶𝘮𝘦𝘯𝘵');
};
if(PyVersion() < 3) {
    # https://techblog.workiva.com/tech-blog/unobfuscating-unicode-ubiquity-practical-guide-unicode-and-utf-8#py-exception-raising
    # says that we shouldn't send back UTF-8 encoded exception messages in
    # Python 2.x, so just accept UTF-8 encoded as "\uXXXX"
    like( Encode::decode_utf8( $@ ), qr/line \d+, in raise_exception\s+Exception: Unicode exception: .+?/, 'Exception raised' );
} else {
    like( Encode::decode_utf8( $@ ), qr/line \d+, in raise_exception\s+Exception: 𝘜𝘯𝘪𝘤𝘰𝘥𝘦 𝘦𝘹𝘤𝘦𝘱𝘵𝘪𝘰𝘯: 𝘜𝘯𝘪𝘤𝘰𝘥𝘦 𝘢𝘳𝘨𝘶𝘮𝘦𝘯𝘵/, 'Exception raised' );

}
