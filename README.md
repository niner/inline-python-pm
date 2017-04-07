# INTRODUCTION

[![Build Status](https://travis-ci.org/berkmancenter/inline-python-pm.svg?branch=travis-ci)](https://travis-ci.org/berkmancenter/inline-python-pm)

`Inline::Python` -- Write Perl subs and classes in Python.

`Inline::Python` lets you write Perl subroutines and classes in
Python. You don't have to use any funky techniques for sharing most
types of data between the two languages, either. `Inline::Python` comes
with its own data translation service. It converts any Python structures
it knows about into Perl structures, and vice versa. 

Example:

    use Inline Python => <<'END';
    def JAxH(x): 
        return "Just Another %s Hacker" % x
    END

    print JAxH('Inline'), "\n";

When run, this complete program prints:

    Just Another Inline Hacker.

The almost-one-line version is:

    perl -le 'use Inline Python=>q{def JAxH(x):return"Just Another %s Hacker"%x};print JAxH+Inline'


# INSTALLATION

This module requires `Inline.pm` version 0.46 or higher to be installed. In 
addition, you need Python 2.5 or greater installed. Python 2.6, Python 3.2 or greater
is recommended.

Python has to be configured with `--enable-shared`. Linux distribution packages
should be fine, but keep it in mind if you compile Python yourself.

To install `Inline::Python` do this:

    perl Makefile.PL
    make
    make test
    make install

(On ActivePerl for MSWin32, use `nmake` instead of `make`.)

You have to `make install` before you can run it successfully.

# INFORMATION:

- For more information on `Inline::Python` see `perldoc Inline::Python`.
- For information about `Inline.pm`, see `perldoc Inline`.
- For information on using Python or the Python C API, visit <http://www.python.org>.

The `Inline::Python` mailing list is <inline@perl.org>. Send mail to
<inline-subscribe@perl.org> to subscribe.

Please send questions and comments to "Stefan Seifert" <NINE@cpan.org>

Copyright (c) 2000, Neil Watkiss. All Rights Reserved. This module is free software.
It may be used, redistributed and/or modified under the same terms as Perl itself.

(see <http://www.perl.com/perl/misc/Artistic.html>)

