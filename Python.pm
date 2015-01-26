package Inline::Python;
use strict;
use Carp;
require Inline;
require DynaLoader;
require Exporter;
our ($VERSION, @ISA, @EXPORT_OK);
@ISA = qw(Inline DynaLoader Exporter);
$VERSION = '0.48';
@EXPORT_OK = qw(py_eval
		py_new_object
		py_call_method 
		py_call_function
                py_is_tuple
		py_bind_class
		py_bind_func
		py_study_package
		eval_python
	       );

# Prevent Inline's import from complaining
sub import {
    Inline::Python->export_to_level(1,@_);
}

sub dl_load_flags { 0x01 }
Inline::Python->bootstrap($VERSION);

#==============================================================================
# Register Python.pm as a valid Inline language
#==============================================================================
sub register {
    return {
	    language => 'Python',
	    aliases => ['py', 'python', 'PYTHON'],
	    type => 'interpreted',
	    suffix => 'pydat',
	   };
}

#==============================================================================
# Validate the Python config options
#==============================================================================
sub validate {
    my $o = shift;

    $o->{ILSM} = {};
    $o->{ILSM}{FILTERS} = [];
    $o->{ILSM}{AUTO_INCLUDE} = {};
    $o->{ILSM}{built} = 0;
    $o->{ILSM}{loaded} = 0;

    while (@_) {
	my ($key, $value) = (shift, shift);

	if ($key eq 'AUTO_INCLUDE') {
	    add_string($o->{ILSM}{AUTO_INCLUDE}, $key, $value, '');
	    warn "AUTO_INCLUDE has not been implemented yet!\n";
	}
	elsif ($key eq 'FILTERS') {
	    next if $value eq '1' or $value eq '0'; # ignore ENABLE, DISABLE
	    $value = [$value] unless ref($value) eq 'ARRAY';
	    my %filters;
	    for my $val (@$value) {
		if (ref($val) eq 'CODE') {
		    $o->add_list($o->{ILSM}, $key, $val, []);
	        }
		else {
		    eval { require Inline::Filters };
		    croak "'FILTERS' option requires Inline::Filters to be installed."
		      if $@;
		    %filters = Inline::Filters::get_filters($o->{API}{language})
		      unless keys %filters;
		    if (defined $filters{$val}) {
			my $filter = Inline::Filters->new($val, 
							  $filters{$val});
			$o->add_list($o->{ILSM}, $key, $filter, []);
		    }
		    else {
			croak "Invalid filter $val specified.";
		    }
		}
	    }
	}
	else {
	    croak "$key is not a valid config option for Python\n";
	}
	next;
    }
}

sub usage_validate {
    return "Invalid value for config option $_[0]";
}

sub add_list {
    my ($ref, $key, $value, $default) = @_;
    $value = [$value] unless ref $value;
    croak usage_validate($key) unless ref($value) eq 'ARRAY';
    for (@$value) {
	if (defined $_) {
	    push @{$ref->{$key}}, $_;
	}
	else {
	    $ref->{$key} = $default;
	}
    }
}

sub add_string {
    my ($ref, $key, $value, $default) = @_;
    $value = [$value] unless ref $value;
    croak usage_validate($key) unless ref($value) eq 'ARRAY';
    for (@$value) {
	if (defined $_) {
	    $ref->{$key} .= ' ' . $_;
	}
	else {
	    $ref->{$key} = $default;
	}
    }
}

sub add_text {
    my ($ref, $key, $value, $default) = @_;
    $value = [$value] unless ref $value;
    croak usage_validate($key) unless ref($value) eq 'ARRAY';
    for (@$value) {
	if (defined $_) {
	    chomp;
	    $ref->{$key} .= $_ . "\n";
	}
	else {
	    $ref->{$key} = $default;
	}
    }
}

#==========================================================================
# Print a short information section if PRINT_INFO is enabled.
#==========================================================================
sub info {
    my $o = shift;
    my $info =  "";

    $o->build unless $o->{ILSM}{built};

    my @functions = @{$o->{ILSM}{namespace}{functions}||[]};
    $info .= "The following Python functions have been bound to Perl:\n"
      if @functions;
    for my $function (sort @functions) {
	$info .= "\tdef $function()\n";
    }
    my %classes = %{$o->{ILSM}{namespace}{classes}||{}};
    $info .= "The following Python classes have been bound to Perl:\n";
    for my $class (sort keys %classes) {
	$info .= "\tclass $class:\n";
	for my $method (sort @{$o->{ILSM}{namespace}{classes}{$class}}) {
	    $info .= "\t\tdef $method(...)\n";
	}
    }

    return $info;
}

#==========================================================================
# Run the code, study the main namespace, and cache the results.
#==========================================================================
sub build {
    my $o = shift;
    return if $o->{ILSM}{cached};

    # Filter the code
    $o->{ILSM}{code} = $o->filter(@{$o->{ILSM}{FILTERS}});

    # Run the code
    py_eval($o->{ILSM}{code});
    $o->{ILSM}{evaluated}++;

    # Study the main namespace
    my %namespace = py_study_package('__main__');

    # Cache the results
    require Inline::denter;
    my $namespace = Inline::denter->new
      ->indent(
	       *namespace => \%namespace,
	       *filtered => $o->{ILSM}{code},
	      );

    $o->mkpath("$o->{API}{install_lib}/auto/$o->{API}{modpname}");

    open PYDAT, "> $o->{API}{location}" or
      croak "Inline::Python couldn't write parse information!";
    print PYDAT $namespace;
    close PYDAT;

    $o->{ILSM}{namespace} = \%namespace;
    $o->{ILSM}{cached}++;
}

#==============================================================================
# Load the code, run it, and bind everything to Perl
#==============================================================================
sub load {
    my $o = shift;
    return if $o->{ILSM}{loaded};

    # Load the code
    open PYDAT, $o->{API}{location} or 
      croak "Couldn't open parse info!";
    my $pydat = join '', <PYDAT>;
    close PYDAT;

    require Inline::denter;
    my %pydat = Inline::denter->new->undent($pydat);
    $o->{ILSM}{namespace} = $pydat{namespace};
    $o->{ILSM}{code} = $pydat{filtered};

    # Run it
    py_eval($o->{ILSM}{code}) unless $o->{ILSM}{evaluated};

    # Bind it all
    py_bind_func($o->{API}{pkg} . "::$_", '__main__', $_)
      for (@{ $o->{ILSM}{namespace}{functions} || [] });
    py_bind_class($o->{API}{pkg} . "::$_", '__main__', $_,
		  @{$o->{ILSM}{namespace}{classes}{$_}})
      for keys %{ $o->{ILSM}{namespace}{classes} || {} };
    $o->{ILSM}{loaded}++;
}

#==============================================================================
# Wrap a Python function with a Perl sub which calls it.
#==============================================================================
sub py_bind_func {
    my $perlfunc = shift;	# What Perl package should the wrapper be in?
    my $pypkg = shift;		# What Python package does it come from?
    my $function = shift;	# What is the name of the Python function?

    my $bind = <<END;
sub $perlfunc {
    unshift \@_, "$pypkg", "$function";
    return &Inline::Python::py_call_function;
}
END

    eval $bind;
    croak $@ if $@;
}

#==============================================================================
# Wrap a Python class in a Perl package. We wrap every method we know about, 
# and we inherit from Inline::Python::Object so the Perverse Python Programmer 
# can still create dynamic methods on-the-fly using its AUTOLOAD.
#==============================================================================
sub py_bind_class {
    my $pkg = shift;
    my $pypkg = shift;
    my $class = shift;
    my @methods = @_;

    my $bind = <<END;
package ${pkg};
\@${pkg}::ISA = qw(Inline::Python::Object);

# We create new objects by invoking the class as a function
sub new {
    splice \@_, 1, 0, "$pypkg", "$class";
    return &Inline::Python::py_new_object;
}

END

    for my $method (@methods) {
	$bind .= <<END;

# Methods are wrapped just as in AUTOLOAD
sub $method {
    splice \@_, 1, 0, "$method";
    return &Inline::Python::py_call_method
}
END
    }

    eval $bind;
    croak $@ if $@;
}

#==============================================================================
# An overridden function to do everything in one bite
# Note: the eval{} catches the case where $_[0] isn't blessed.
#==============================================================================
sub eval_python {
    return &py_call_method	if eval{$_[0]->isa("Inline::Python::Object")};
    return &py_eval		if @_ == 1 || $_[1] =~ /^\d+$/;
    return &py_call_function	if @_ >= 2;
    croak "Invalid use of eval_python. See 'perldoc Inline::Python'";
}

#==============================================================================
# A more pleasing name than py_call_function, which is what really happens
#==============================================================================
sub py_new_object {
    return &Inline::Python::Object::new;
}

#==============================================================================
# We provide Inline::Python::Object as a base class for Python objects. It
# knows how to create, destroy, and call methods on objects.
#==============================================================================
package Inline::Python::Object;

use overload '%{}' => \&__data__, '""' => \&__inline_str__, fallback => 1;

sub new {
    my $perlpkg = shift;
    return bless &Inline::Python::py_call_function, $perlpkg;
}

sub __data__ {
    my ($self) = @_;

    tie my %data, 'Inline::Python::Object::Data', $self;
    return \%data;
}

sub __inline_str__ {
    my ($self) = @_;

    return Inline::Python::py_has_attr($self, '__str__') ? $self->__str__() : $self;
}

sub AUTOLOAD {
    no strict;
    $AUTOLOAD =~ s|.*::(\w+)|$1|;
    splice @_, 1, 0, $AUTOLOAD;
    return &Inline::Python::py_call_method;
}

# avoid AUTOLOAD warning
sub DESTROY {
}

package Inline::Python::Object::Data;

sub new {
    my $class = shift;
    return $class->TIEHASH(@_);
}

sub TIEHASH {
    my ($class, $self) = @_;
    return bless \$self, $class;
}

sub FETCH {
    my ($self, $key) = @_;

    return Inline::Python::py_get_attr($$self, $key);
}

sub STORE {
    my ($self, $key, $value) = @_;

    return Inline::Python::py_set_attr($$self, $key, $value);
}

package Inline::Python::Function;

use overload '&{}' => \&call, fallback => 1;

sub call {
    my $self = shift;
    return sub { Inline::Python::py_call_function_ref($$self, @_) };
}

package Inline::Python::Boolean;
use overload bool => \&bool, '0+' => \&bool, '!' => \&negate, fallback => 1;

our $true  = __PACKAGE__->new(1);
our $false = __PACKAGE__->new(0);

sub new {
    my ($class, $value) = @_;
    return bless \$value, $class;
}

sub bool {
    my ($self) = @_;
    return $$self;
}

sub negate {
    my ($self) = @_;
    return $self ? $false : $true;
}

1;
