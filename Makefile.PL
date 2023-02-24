use Data::Dumper;
use Config;
use Cwd qw(abs_path);
use ExtUtils::MakeMaker 6.64;
use Getopt::Long;
use File::Spec;
use File::Basename;

GetOptions(
	   'gdb:s' => \$gdb,
	   debug => \$debug,
	   help => \$help,
	  );

usage() if $help;

#============================================================================
# What python are we going to try?
#============================================================================
my $sel = $ENV{INLINE_PYTHON_EXECUTABLE};
my $num_python3 = 0;
unless ($sel) {
    my @pythons;
    my %pythons;
    my $sep = $^O eq 'MSWin32' ? ";" : ":";
    my $exe = $^O eq 'MSWin32' ? ".exe" : "";
    for $p (split /$sep/, $ENV{PATH}) {
        $p =~ s/^~/$ENV{HOME}/;
        for $exe_version ('3', '') {
            my $py = File::Spec->catfile($p, "python$exe_version$exe");
            next unless -f $py and -x $py;
            next if $pythons{abs_path($py)}++; # filter symlinked duplicates
            my $version = get_python_major_version($py);
            push @pythons, { path => $py, version => $version };
            $num_python3++ if ($version == 3);
        }
    }

    # Keep them in PATH order.
    # @pythons = sort { $a->{path} cmp $b->{path} } @pythons;

    my $num = 1;
    print "Found these python executables on your PATH:\n";
    print $num++ . ". " . $_->{path} . "\n" for @pythons;

    if (@pythons == 1 and not $sel) {
        $sel = $pythons[0];
        print "Using the only python executable I could find\n";
        print 'Set the INLINE_PYTHON_EXECUTABLE environment variable to'
            . " the full path to your python executable to override this selection.\n";
    } elsif ($num_python3 == 1) {   # Prefer python 3
        for my $python (@pythons) {
            if ($python->{version} == 3) {
                $sel = $python;
                print "Using the only python 3 executable I could find even though python 2 was also found\n";
                print 'Set the INLINE_PYTHON_EXECUTABLE environment variable to'
                    . " the full path to your python executable to override this selection.\n";
                last;
            }
        }
    }
    unless ($sel) {
        $sel = prompt("Use which?", '1');
        if ($sel =~ /^\d+$/) {
            die 'Invalid number. Please enter only numbers from 1 to ' . ($num - 1)
                . " or the full path to your python executable.\n"
                . 'Set the INLINE_PYTHON_EXECUTABLE environment variable to'
                . " the full path to your python executable to avoid this question.\n"
                if $sel > ($num - 1);
            $sel = $pythons[$sel - 1];
        }
    }
}
$sel = { path => $sel } unless ref $sel eq 'HASH'; # in case the user entered a path

print "Using $sel->{path}\n";
my $py_major_version = get_python_major_version($sel->{path});

#============================================================================
# Interrogate the python interpreter (or the user) for required flags
#============================================================================
interrogate($sel);

# Fix up the libpath and libpython
die "Could not find Python.h in include path. make will not work"
  unless -e File::Spec->catfile($sel->{incpath}, "Python.h");

substr($sel->{incpath}, 0, 0) = "-I";
substr($sel->{libpath}, 0, 0) = "-L";
$sel->{libpython} =~ s/lib(.*)(?:\.\Q$Config{dlext}\E|\Q$Config{_a}\E)/-l$1/;

my @flags;
push @flags, debug_flag() if defined $gdb;
push @flags, '-DI_PY_DEBUG' if $debug;
push @flags, "-DPY_MAJOR_VERSION=$py_major_version";
push @flags, 'none (perl Makefile.PL --help for details)' unless @flags;
$sel->{syslib} = '' if $sel->{syslib} eq "None";
print <<END;
Using These Settings:
   Extra Libs:  $sel->{syslibs}
   Python Lib:  $sel->{libpath} $sel->{libpython}
   Includes:    $sel->{incpath}
   Extra Flags: @flags
END

#============================================================================
# Finalize, and write the makefile
#============================================================================
$defs = join ' ', qw(-DEXPOSE_PERL -DCREATE_PYTHON -UCREATE_PERL),
	$debug ? "-DI_PY_DEBUG" : ();

WriteMakefile(
	      $defs ? (DEFINE => $defs) : (),
	      defined $gdb  ? (OPTIMIZE => debug_flag()) : (),
	      INC => $sel->{incpath},
	      LIBS => (join " ", @$sel{qw(libpath libpython syslibs)}),
	      NAME => 'Inline::Python',
              ABSTRACT_FROM => 'Python.pod',
              AUTHOR => 'Neil Watkiss <NEILW@cpan.org>',
              LICENSE => 'perl',
	      VERSION_FROM => 'Python.pm',
		  MIN_PERL_VERSION => '5.6.0',
	      PREREQ_PM => {
                            'Inline'       => 0.46,
                            'Digest::MD5'  => 2.50,
                            'Data::Dumper' => 0,
                            'File::Spec'   => 0,
			  },
        TEST_REQUIRES => {
                            'Test'                => 0,
                            'Test::More'          => 0,
                            'Test::Deep'          => 0,
                            'Test::Number::Delta' => 0,
                            'Proc::ProcessTable'  => '0.53',
        },
	      OBJECT => 'Python.o py2pl.o perlmodule.o util.o',
	      META_MERGE => {
			    "meta-spec" => { version => 2 },
			    resources => {
			     repository => {
			      type => 'git',
			      url => 'http://github.com/niner/inline-python-pm.git',
			      web => 'http://github.com/niner/inline-python-pm',
			     },
			    },
			   },
	      clean => {FILES => 'blib_test/'},
         dynamic_lib => {
             OTHERLDFLAGS => ($sel->{rpath} ? "-Wl,-rpath,$sel->{rpath}" : ''),
          },

	     );

#============================================================================
# Tries to ask the python interpreter what libraries we need, where its
# include directories are, etc.
# Typical values of the Python sysconfig variables:
#
# - VERSION
#
#   - Ubuntu : 3.10
#   - Windows: 310
#   - macOS  : 3.10
#
# - BINDIR
#
#   - Ubuntu :
#    - system version: /usr/bin
#    - pyenv version : /home/username/.pyenv/versions/3.9.4/bin
#  - Windows : C:\Python\Python310  (NOTE: missing trailing "bin")
#  - macOS   :
#    - system version : /Applications/Xcode.app/Contents/Developer/Library
#                        /Frameworks/Python3.framework/Versions/3.8/bin
#    - pyenv version  : /Users/username/.pyenv/versions/3.10.0-debug/bin
#
# - LIBS:
#
#   - Ubuntu : -lcrypt -lpthread -ldl  -lutil -lm
#   - Windows: [None]
#   - macOS  :
#    - system version : -ldl -lSystem -framework CoreFoundation
#    - pyenv version  : -ldl -framework CoreFoundation
#
# - INCLUDEPY
#
#   - Ubuntu : /usr/include/python3.9
#   - Windows: C:\Python\Python310\Include
#   - macOS
#    - system version : /Applications/Xcode.app/Contents/Developer/Library
#                        /Frameworks/Python3.framework/Versions/3.8/Headers
#    - pyenv version  : /Users/username/.pyenv/versions/3.10.0-debug/include/python3.10d
#
# - LIBPL
#
#   - Ubuntu :
#    - system version : /usr/lib/python3.9/config-3.9-x86_64-linux-gnu
#    - pyenv version  : /home/username/.pyenv/versions/3.8.9
#                         /lib/python3.8/config-3.8-x86_64-linux-gnu (NOTE: this folder does
#                              not contain a shared library even if python was built with
#                              --enable-shared. However, the folder LIBDIR, see below, does)
#   - Windows: [None]  (NOTE: this should be "$BINDIR/libs" on windows)
#   - macOS  :
#    - system version : /Applications/Xcode.app/Contents/Developer/Library/Frameworks
#                        /Python3.framework/Versions/3.8/lib/python3.8/config-3.8-darwin
#    - pyenv version  : /Users/username/.pyenv/versions/3.10.0-debug/lib
#                           /python3.10/config-3.10d-darwin
#
# - LDLIBRARY
#
#   - Ubuntu :
#     - if python was built with --enable-shared : libpython3.9.so
#     - else                                     : libpython3.9.a
#   - Windows: [None] (NOTE: this should be "python310.lib" on windows,
#                      where a .lib file is a so-called import-library on Windows. The
#                      import library reference a .dll library in $BINDIR.
#                      On Windows there is also a stable-across-versions-subset library
#                      called "python3.lib" (which references "python3.dll" in $BINDIR,
#                      see https://www.python.org/dev/peps/pep-0384/ for more information.)
#   - macOS  :
#    - system version: Python3.framework/Versions/3.8/Python3 (NOTE: this is a .dylib
#                           i.e. a shared library)
#    - pyenv version :
#     - if built with --enable-shared : libpython3.10d.dylib
#     - else                          : libpython3.10d.a
#
# - LIBRARY
#
#   - Ubuntu : libpython3.9.a
#   - Windows: [None]  (NOTE: static library is not available but import library exists
#                        see note on LDLIBRARY above)
#   - macOS  : libpython3.10d.a
#
# - LIBDEST
#
#   - Ubuntu : /home/username/.pyenv/versions/3.9.4/lib/python3.9
#   - Windows: C:\Python310\Lib (NOTE: this folder does not contain anything interesting
#                            to us. No static or shared libraries here, but the import
#                            library is in the libs folder C:\Python310\libs and the
#                            .dll library is in the BINDIR C:\Python310)
#   - macOS  :
#    - system version : /Applications/Xcode.app/Contents/Developer/Library
#                          /Frameworks/Python3.framework/Versions/3.8/lib/python3.8
#    - pyenv version  : /Users/username/.pyenv/versions/3.10.0-debug/lib/python3.10
#
# - LIBDIR
#
#   - Ubuntu  :
#    - system version : /usr/lib/x86_64-linux-gnu
#    - pyenv version  : /home/username/.pyenv/versions/3.9.4/lib
#   - Windows : [None]
#   - macOS   :
#    - system version: /Applications/Xcode.app/Contents/Developer/Library
#                          /Frameworks/Python3.framework/Versions/3.8/lib
#    - pyenv version : /Users/username/.pyenv/versions/3.10.0-debug/lib
#
#============================================================================
sub interrogate {
    my $ref = shift;
    return query_options($ref) unless test_interrogate($ref);
    $ref->{syslibs}   = get_config_var($ref, "LIBS");
    $ref->{incpath}   = get_config_var($ref, "INCLUDEPY");
    $ref->{libpath}   = get_config_var($ref, "LIBPL");
    $ref->{ldlib}     = get_config_var($ref, "LDLIBRARY");
    $ref->{libpython} = get_config_var($ref, "LIBRARY");
    my $tmp = File::Spec->canonpath($ref->{libpython});
    my @dirs = File::Spec->splitdir( $tmp );
    $ref->{libpython} = $dirs[-1];
    # On Windows, Python config var "LIBRARY" is not defined
    if ($ref->{libpython} eq 'None') {
        special_get_libpath($ref);
    }
    $ref->{libpath} = File::Spec->catfile(get_config_var($ref, "LIBDEST"), 'config')
        if ($ref->{libpath} eq 'None');
    $ref->{rpath} = ''; # only used if we are linking with a shared library, see below.
    $ref->{cflags} = get_config_var($ref, 'CFLAGS');
    $ref->{config_args} = get_config_var($ref, 'CONFIG_ARGS');
    $ref->{enable_shared} = (get_config_var($ref, 'Py_ENABLE_SHARED') eq "1");
    if (using_macos_system_python($ref)) {
        add_rpath_for_macos_system_python($ref);
    }
    elsif ($^O ne "MSWin32") {  # we use the import library on Windows,
                                   # see special_get_libpath() below
        special_non_windows_check_shared_static_libs($ref);
    }
    return query_options($ref) unless sanity_check($ref);
}

# on macOS using the system python, the path to the shared and static libraries
#   is given by "libpath" (LIBPL). In this directory there exists two files:
#   - libpythonxxx.a
#   - libpythonxxx.dylib
# the first name (the static library libpythonxxx.a) is given by the python sysconfig
# variable "libpython" (LIBRARY). However, both these files are symlinks to a shared
# library called "Python" which is located relative to LIBPL with path:
# ../../../Python3. This file "Python3" (or "Python2" ??), is a dylib with an embedded
#  @rpath magic search path given by: @rpath/Python3.framework/Versions/3.8/Python3 which
#  is also the ID of the library (LC_ID_DYLIB) which means that Python.so (the Perl
#  generated interface) must include an rpath to the directory 3 levels above the
# location of "Python3" (which is 6 levels above libpythonxxx.dylib in LIBPL).
#  This directory is fortunately given by the config variable
#  PYTHONFRAMEWORKPREFIX.
#
sub add_rpath_for_macos_system_python {
    my ($ref) = @_;

    $ref->{rpath} = get_config_var($ref, 'PYTHONFRAMEWORKPREFIX');
};

sub special_non_windows_check_shared_static_libs {
    my ($ref) = @_;

    if (shared_lib_priority($ref)) {
        $ref->{libpython} = $ref->{ldlib};
        $ref->{rpath} = $ref->{libpath};
    }
    else {
        if (!check_static_library_ok($ref)) {
            # In this case we may find a shared library to link with in
            #  $LIBDIR instead of in $LIBPL, this happens if you install Python
            #  with pyenv (on the other hand for the system python there will
            #  a shared library in both $LIBDIR and $LIBPL, see issue #29 for more information.
            # TODO: However, this still does not work for pyenv (tested on Ubuntu). For some
            #   reason this shared library does not behave well unless python was also compiled
            #   with -fPIC option.
            if (( $ref->{cflags} !~ /\Q-fPIC\E/i)  && ($ref->{path} !~ m{^/(?:usr/)?bin/python})) {
                if ($ref->{enable_shared}) {
                    warn "WARNING: This python's shared library was compiled with --enable-shared but not "
                    ." with -fPIC option, this might lead to strange runtime behavior.\n";
                }
                else {
                    # TODO: strangely this seems to work fine on macOS.
                    #   More investigation is needed here...
                    warn "WARNING: This python was not compiled with --enable-shared and not "
                        . "with -fPIC.\n"
                        . "WARNING: This is known to not work on linux.\n";
                }
            }
            $ref->{libpython} = $ref->{ldlib};
            my $shared_lib = File::Spec->catfile($ref->{libpath}, $ref->{libpython});
            if (!-f $shared_lib) {
                $ref->{libpath} = get_config_var($ref, "LIBDIR");
            }
            else {
                $ref->{rpath} = $ref->{libpath};
            }
        }
    }
}

sub shared_lib_priority {
    my ($ref) = @_;

    if ($ref->{libpython} ne $ref->{ldlib}) {
        # This should happen if python was compiled with --enable-shared
        #  In this case the linker will prefer the shared library
        my $static_lib = File::Spec->catfile($ref->{libpath}, $ref->{libpython});
        my $shared_lib = File::Spec->catfile($ref->{libpath}, $ref->{ldlib});
        return 1 if (-f $static_lib) && (-f $shared_lib);
    }
    return 0;
}

sub check_static_library_ok {
    my ($ref) = @_;

    my $static_lib = File::Spec->catfile($ref->{libpath}, $ref->{libpython});
    return 0 if (!-f $static_lib);

    # We should check if the static library was compiled with -fPIC, or else
    #  we cannot create a shared Python.so from it.
    #  TODO: It seem like it is possible to build python with --enable-shared
    #    and without CFLAGS=-fPIC, and this will make both libpythonxx.so and
    #    libpythonxx.a  position independent, but in this case strange things
    #    may happen at runtime (i.e. when running "make test"), see issue #29
    #    for more information. This should be investigated futher to determine what
    #    is actually going on.
    #    This seems to not be a problem on macOS though.
    return 1 if $ref->{cflags} =~ /\Q-fPIC\E/i;
    warn "WARNING: The static python library seems not to be position indepenent.\n"
         . "WARNING: If this does not work you should try again with a "
         . "python version that was compiled with CFLAGS=-fPIC\n";
    return 0;
}

sub using_macos_system_python {
    my ($ref) = @_;

    return ($^O eq "darwin") && ($ref->{path} =~ m{^/usr/bin/python});
}

sub check_shared_lib_support() {
    my ($ref) = @_;

    # Windows python always have a shared lib
    return 1 if $^O eq "MSWin32";
    # The system python always have a shared lib on macOS
    return 1 if using_macos_system_python($ref);
    # See https://stackoverflow.com/a/23202055/2173773
    return $ref->{enable_shared};
}

sub get_python_version {
    my $ref = shift;
    my $major = `$ref->{path} -c "import sys; print(sys.version_info[0])"`;
    my $minor = `$ref->{path} -c "import sys; print(sys.version_info[1])"`;
    return ($major, $minor);
}

# On Windows, Python config var "LIBRARY" is not defined, so we try another method
#  to obtain the library path name
sub special_get_libpath {
    my $ref = shift;
    my ($major, $minor) = get_python_version($ref);
    my $cmd;
    if (($major == 3 && $minor >=10) || $major > 3 ) {
        $cmd = 'import setuptools.command.build_ext; d=setuptools.dist.Distribution();'
              .'b=setuptools.command.build_ext.build_ext(d)';
    }
    else {
        $cmd = 'import distutils.command.build_ext; d=distutils.core.Distribution();'
              . 'b=distutils.command.build_ext.build_ext(d)';
    }
    my @lines = `$ref->{path} -c "$cmd;b.finalize_options();print(b.library_dirs[0])" 2>&1`;
    my $val = $lines[-1];
    chomp $val;
    return '' if !$val;
    # On Windows, $val should now be equal to $BINDIR/libs
    my $pyscript = "import sysconfig; "
                  . "print(sysconfig.get_config_var('VERSION'))";
    my $version = `$ref->{path} -c "$pyscript"`;
    chomp $version;
    $ref->{libpath} = $val;
    $ref->{libpython} = "python${version}.lib"; # Note: on Windows this is an import library,
                                                #  that referes to a shared library
                                                #  (not a static library)
    # The above file should always exist, alternatively we could
    #  set libpath to $BINDIR and libpython to python$version.dll and use the shared library
    #  directly instead of using the import library.
    return $val;
}

sub test_interrogate {
    my $ref = shift;
    `$ref->{path} -c "import sysconfig; sysconfig.get_config_var" 2>&1`;
    print <<END if $?;

This python is so old it doesn't know how to answer my questions.

Instead, you will be asked a series of questions about it. If possible,
I will give you a set of reasonable options to choose from. You can
always enter the complete answer yourself if none of mine are correct.
END
    #' stupid vim.
    return $? == 0 ? 1 : 0;
}

sub sanity_check {
    my $ref = shift;

    $ref->{libpython} = $ref->{ldlib}
        if not -f File::Spec->catfile($ref->{libpath}, $ref->{libpython})
           and -f File::Spec->catfile($ref->{libpath}, $ref->{ldlib});
    my $libpath = File::Spec->catfile($ref->{libpath}, $ref->{libpython});
    unless (-d $ref->{libpath} &&
	    -d $ref->{incpath} &&
	    (-f File::Spec->catfile($ref->{libpath}, $ref->{libpython}))
	   ) {
	print <<END and return 0;

This python's configuration files are messed up. You'll have have to
answer the questions yourself. Here is what Python said:

   Extra Libs:  $ref->{syslibs}
   Python Library:  $libpath
   Include Path:    $ref->{incpath}
END
    # ' stupid vim.
    }
    return 1;
}

sub get_python_major_version {
    my $exe = shift;
    my $version = `$exe --version 2>&1`;

    $version =~ /(\d+)\./;
    return $1;
}

sub get_config_var {
    my $ref = shift;
    my $key = shift;
    my $exe = $ref->{path};
    my $val = `$exe -c "import sysconfig; print(sysconfig.get_config_var('$key'))"`;
    chomp $val;
    return $val;
}

sub get_default_python_lib_index {
  my $r_found_libs=shift;

  # Avoid limited API libraries (ABI), which are versionned with 1 digit only
  # (cf https://docs.python.org/3/c-api/stable.html)
  for my $i (0..$#{$r_found_libs}) {
    return ($i+1) if($r_found_libs->[$i] =~ /python\d\.?\d[^\/\\]+$/i);
  }
  
  return 1;
}

sub query_options {
    my $ref = shift;

    # Every python I've seen needs pthreads. Obviously not on windows.
    my $libs_guess = $ref->{syslibs} ? $ref->{syslibs} :
		     $^O eq 'MSWin32' ? '' : '-lpthread';
    print <<END;

	1. LIBS option. I need to know what extra libraries, if any,
	   are required by this build of python. I recommend this:
	   ${ $libs_guess ? \$libs_guess : \"No extra libraries" }

END
    $ref->{syslibs} = prompt("Enter extra libraries (e.g. -lfoo -lbar)",
			     $libs_guess);

    print <<END;

	2. LIBRARY option. The location of the python library.
	   Inline::Python needs to link against it to use Python.

	Here are the libraries I know about:
END
    my @libs = show_python_libs($ref);
    my $defaultLibIndex = $^O eq 'MSWin32' ? get_default_python_lib_index(\@libs) : 1;
    my $lib = prompt("Which? Or enter another.", $defaultLibIndex);
    $lib = $libs[$lib-1] if $lib =~ /^\d+$/;
    $lib =~ s|\\|/|g;
    my ($volume, $directories, $file) = File::Spec->splitpath( $lib );
    $ref->{libpath} = File::Spec->canonpath(File::Spec->catpath($volume, $directories));
    $ref->{libpython} = $file;

    print <<END;

	3. INCLUDE option. The location of the python include files.
	   Inline::Python needs these to compile.

	Here are the locations I know about:
END
    my @incs = show_python_incs($ref);
    my $inc  = prompt("Which? Or enter another.", '1');
    $inc = $incs[$inc-1] if $inc =~ /^\d+$/;
    $ref->{incpath} = $inc;
}

#============================================================================
# Python libraries to look for
#============================================================================
sub show_python_libs {
  my $ref = shift;
  my $exe = $ref->{path};

  # Convert the exe into a glob where we might find a library:
  $exe =~ s|[^/]+$||;
  $exe .= "../lib/python*/config/libpython*";

  my @py_libs =
   (
   (map { $exe . $_ } '.a', '.so', '.lib'),
   '/usr/lib64/libpython*.a',
   '/usr/lib64/libpython*.so',
   '/usr/lib/libpython*.a',
   '/usr/lib/libpython*.so',
   '/usr/lib64/python*/config/libpython*.a',
   '/usr/lib64/python*/config/libpython*.so',
   '/usr/lib/python*/config/libpython*.a',
   '/usr/lib/python*/config/libpython*.so',
   '/usr/local/lib/libpython*.a',
   '/usr/local/lib/libpython*.so',
   '/usr/local/ActivePython-*/lib/python*/config/libpython*.a',
   '/usr/local/ActivePython-*/lib/python*/config/libpython*.so',

   # Win32 support
   'C:/Python*/libs/python*.lib',
   'C:/Program Files/Python*/libs/python*.lib',
  );

  my (@found, %found);
  push @found, grep { -f && $found{abspath($_)}++ == 0 } glob for @py_libs;
  @found = sort map { abspath($_) } @found;
  my $num = '1';
  print "\t   " . $num++ . ") " . $_ . "\n" for @found;
  print "\n";
  return @found;
}

#============================================================================
# Python include files to look for
#============================================================================
sub show_python_incs {
  my $ref = shift;
  my $exe = $ref->{path};

  # Convert the exe into a glob where we might find the includes:
  $exe =~ s|[^/]+$||;
  $exe .= "../include/python*";

  my @py_incs;
  if ($^O eq "MSWin32") {
    @py_incs = (
      'C:\Python*\include',
      'C:\Program Files\Python*\include'
    );
  }
  else {
    @py_incs = (
      $exe,
      '/usr/local/ActivePython-*/include/python*',
      '/usr/include/python*',
      '/usr/local/include/python*',
    );
  }

  my (@found, %found);
  push @found, grep { -d && $found{abspath($_)}++ == 0 } glob for @py_incs;
  @found = sort map { abspath($_) } @found;
  my $num = 1;
  print "\t   " . $num++ . ") " . $_ . "\n" for @found;
  print "\n";
  return @found;
}

# This can deal with files as well as directories
sub abspath {
    use Cwd qw(abs_path);
    my ($path, $file) = shift;
    if (-f $path) {
        my @parts = File::Spec->splitpath($path);
        $path = File::Spec->canonpath(File::Spec->catpath(@parts[0..1]));
    	$file = $parts[-1];
    }
    $path = abs_path($path);
    return defined $file ? File::Spec->catfile($path, $file) : $path;
}

sub debug_flag {
    return $gdb if $gdb;
    $Config{osname} eq 'MSWin32' 	? return '-Zi' : return '-g';
}

sub usage {
    print <<'END';
Options:
    -gdb:   Turn on compiler's debugging flag (use my guess).
    -gdb=x  Pass your own debugging flag, not mine.
    -debug: Turn on many diagnostic print statements inside Inline::Python.
            This option is useful for tracing the execution path when
            debugging.
    -help:  This output.
END
# ' stupid vim
    exit 0;
}
