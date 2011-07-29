use Data::Dumper;
use Config;
use ExtUtils::MakeMaker;
use Getopt::Long;

GetOptions(
	   'gdb:s' => \$gdb,
	   debug => \$debug,
	   help => \$help,
	  );

usage() if $help;

#============================================================================
# What python are we going to try?
#============================================================================
my @pythons;
my $sep = $^O eq 'MSWin32' ? ";" : ":";
for $p (split /$sep/, $ENV{PATH}) {
    $p =~ s/^~/$ENV{HOME}/;
    $p .= "/python";
    push @pythons, { path => $p } if -f $p && -x $p;
}

# Keep them in PATH order.
#@pythons = sort { $a->{path} cmp $b->{path} } @pythons;

my $num = 1;
print "Found these python executables on your PATH:\n";
print $num++ . ". " . $_->{path} . "\n" for @pythons;

my $sel = prompt("Use which?", '1');
$sel = $pythons[$sel-1] if $sel =~ /^\d+$/;
$sel = { path => $sel } unless ref $sel eq 'HASH';

print "Using $sel->{path}\n";

#============================================================================
# Interrogate the python interpreter (or the user) for required flags
#============================================================================
interrogate($sel);

# Fix up the libpath and libpython
die "Could not find Python.h in include path. make will not work" unless -e "$sel->{incpath}/Python.h";
substr($sel->{incpath}, 0, 0) = "-I";
substr($sel->{libpath}, 0, 0) = "-L";
$sel->{libpython} =~ s/lib(.*)(?:\.\Q$Config{dlext}\E|\Q$Config{_a}\E)/-l$1/;

my @flags;
push @flags, debug_flag() if defined $gdb;
push @flags, '-DI_PY_DEBUG' if $debug;
push @flags, 'none (perl Makefile.PL --help for details)' unless @flags;
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
	      PREREQ_PM => {
                            Inline         => 0.46,
                            'Digest::MD5'  => 2.50,
                            'Data::Dumper' => 0,
                            'Test::More'   => 0,
                            Test           => 0,
			   },
	      OBJECT => 'Python.o py2pl.o perlmodule.o util.o',
	      clean => {FILES => 'blib_test/'},
	     );

#============================================================================
# Tries to ask the python interpreter what libraries we need, where its
# include directories are, etc.
#============================================================================
sub interrogate {
    my $ref = shift;
    return query_options($ref) unless test_interrogate($ref);
    $ref->{syslibs}   = get_config_var($ref, "LIBS");
    $ref->{incpath}   = get_config_var($ref, "INCLUDEPY");
    $ref->{libpath}   = get_config_var($ref, "LIBPL");
    $ref->{libpython} = get_config_var($ref, "LIBRARY");
    my $tmp = rindex($ref->{libpython}, '/') + 1;
    $ref->{libpython} = substr($ref->{libpython}, $tmp);
    $ref->{libpath} = join '/', (get_config_var($ref, "LIBDEST"),
				 'config')
      if ($ref->{libpath} eq 'None');
    return query_options($ref) unless sanity_check($ref);
}

sub test_interrogate {
    my $ref = shift;
    `$ref->{path} -c "import distutils.sysconfig; distutils.sysconfig.get_config_var" 2>&1`;
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
    unless (-d $ref->{libpath} &&
	    -d $ref->{incpath} &&
	    (-f join '/', $ref->{libpath}, $ref->{libpython})
	   ) {
	print <<END and return 0;

This python's configuration files are messed up. You'll have have to
answer the questions yourself. Here is what Python said:

   Extra Libs:  $sel->{syslibs}
   Python Library:  $sel->{libpath}/$sel->{libpython}
   Include Path:    $sel->{incpath}
END
    # ' stupid vim.
    }
    return 1;
}

sub get_config_var {
    my $ref = shift;
    my $key = shift;
    my $exe = $ref->{path};
    my $val = `$exe -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('$key')"`;
    chomp $val;
    return $val;
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
    my $lib = prompt("Which? Or enter another.", '1');
    $lib = $libs[$lib-1] if $lib =~ /^\d+$/;
    $lib =~ s|\\|/|g;
    $ref->{libpath} = substr($lib, 0, rindex($lib, '/'));
    $ref->{libpython} = substr($lib, rindex($lib, '/')+1);

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
   '/usr/lib/libpython*.a',
   '/usr/lib/libpython*.so',
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

  my @py_incs = 
   (
   $exe,
   '/usr/local/ActivePython-*/include/python*',
   '/usr/include/python*',
   '/usr/local/include/python*',

   # Win32 support
   'C:/Python*/include',
   'C:/Program Files/Python*/include',
   );

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
	my @p = split '/', $path;
        $path = join '/', @p[0..$#p-1]; # can't use -2 in a range
	$file = $p[-1];
    }
    $path = abs_path($path);
    return defined $file ? join '/', $path, $file : $path;
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
