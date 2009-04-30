#!/usr/bin/perl -w
#
# Copywrite 2005-2009 - Steven Rostedt
# Licensed under the terms of the GNU GPL License version 2
#
#  It's simple enough to figure out how this works.
#  If not, then you can ask me at stripconfig@goodmis.org
#
# What it does?
#
#   If you have installed a Linux kernel from a distribution
#   that turns on way too many modules than you need, and
#   you only want the modules you use, then this program
#   is perfect for you.
#
#   It gives you the ability to turn off all the modules that are
#   not loaded on your system.
#
# Howto:
#
#  1. Boot up the kernel that you want to stream line the config on.
#  2. Change directory to the directory holding the source of the
#       kernel that you just booted.
#  3. Copy the configuraton file to this directory as .config
#  4. Have all your devices that you need modules for connected and
#      operational (make sure that their corresponding modules are loaded)
#  5. Run this script redirecting the output to some other file
#       like config_strip.
#  6. Back up your old config (if you want too).
#  7. copy the config_strip file to .config
#  8. Run "make oldconfig"
#
#  Now your kernel is ready to be built with only the modules that
#  are loaded.
#
# Here's what I did with my Debian distribution.
#
#    cd /usr/src/linux-2.6.10
#    cp /boot/config-2.6.10-1-686-smp .config
#    ~/bin/streamline_config > config_strip
#    mv .config config_sav
#    mv config_strip .config
#    make oldconfig
#
my $config = ".config";
my $linuxpath = ".";

open(CIN,$config) || die "Can't open current config file: $config";
my @makefiles = `find $linuxpath -name Makefile`;
my %depends;
my %selects;
my %prompts;
my %objects;
my $var;
my $cont = 0;

# Get the top level Kconfig file (passed in)
my $kconfig = $ARGV[0];

# prevent recursion
my %read_kconfigs;

sub read_kconfig {
    my ($kconfig) = @_;

    my $state = "NONE";
    my $config;
    my @kconfigs;

    open(KIN, $kconfig) || die "Can't open $kconfig";
    while (<KIN>) {
	chomp;

	# collect any Kconfig sources
	if (/^source\s*"(.*)"/) {
	    $kconfigs[$#kconfigs+1] = $1;
	}

	# configs found
	if (/^\s*config\s+(\S+)\s*$/) {
	    $state = "NEW";
	    $config = $1;

	# collect the depends for the config
	} elsif ($state eq "NEW" && /^\s*depends\s+on\s+(.*)$/) {
	    $state = "DEP";
	    $depends{$config} = $1;
	} elsif ($state eq "DEP" && /^\s*depends\s+on\s+(.*)$/) {
	    $depends{$config} .= " " . $1;

	# Get the configs that select this config
	} elsif ($state ne "NONE" && /^\s*select\s+(\S+)/) {
	    if (defined($selects{$1})) {
		$selects{$1} .= " " . $config;
	    } else {
		$selects{$1} = $config;
	    }

	# configs without prompts must be selected
	} elsif ($state ne "NONE" && /^\s*tristate\s\S/) {
	    # note if the config has a prompt
	    $prompt{$config} = 1;

	# stop on "help"
	} elsif (/^\s*help\s*$/) {
	    $state = "NONE";
	}
    }
    close(KIN);

    # read in any configs that were found.
    foreach $kconfig (@kconfigs) {
	if (!defined($read_kconfigs{$kconfig})) {
	    $read_kconfigs{$kconfig} = 1;
	    read_kconfig($kconfig);
	}
    }
}

if ($kconfig) {
    read_kconfig($kconfig);
}

# Read all Makefiles to map the configs to the objects
foreach my $makefile (@makefiles) {
    chomp $makefile;

    open(MIN,$makefile) || die "Can't open $makefile";
    while (<MIN>) {
	my $objs;

	# is this a line after a line with a backslash?
	if ($cont && /(\S.*)$/) {
	    $objs = $1;
	}
	$cont = 0;

	# collect objects after obj-$(CONFIG_FOO_BAR)
	if (/obj-\$\((CONFIG_[^\)]*)\)\s*[+:]?=\s*(.*)/) {
	    $var = $1;
	    $objs = $2;
	}
	if (defined($objs)) {
	    # test if the line ends with a backslash
	    if ($objs =~ m,(.*)\\$,) {
		$objs = $1;
		$cont = 1;
	    }

	    foreach my $obj (split /\s+/,$objs) {
		$obj =~ s/-/_/g;
		if ($obj =~ /(.*)\.o$/) {
		    # Objects may bes enabled by more than one config.
		    # Store configs in an array.
		    my @arr;

		    if (defined($objects{$1})) {
			@arr = @{$objects{$1}};
		    }

		    $arr[$#arr+1] = $var;

		    # The objects have a hash mapping to a reference
		    # of an array of configs.
		    $objects{$1} = \@arr;
		}
	    }
	}
    }
    close(MIN);
}

my %modules;

# see what modules are loaded on this system
open(LIN,"/sbin/lsmod|") || die "Cant lsmod";
while (<LIN>) {
	next if (/^Module/);  # Skip the first line.
	if (/^(\S+)/) {
		$modules{$1} = 1;
	}
}
close (LIN);

# add to the configs hash all configs that are needed to enable
# a loaded module.
my %configs;
foreach my $module (keys(%modules)) {
    if (defined($objects{$module})) {
	@arr = @{$objects{$module}};
	foreach my $conf (@arr) {
	    $configs{$conf} = $module;
	}
    } else {
	# Most likely, someone has a custom (binary?) module loaded.
	print STDERR "$module config not found!!\n";
    }
}

my $valid = "A-Za-z_0-9";
my $repeat = 1;

#
# Note, we do not care about operands (like: &&, ||, !) we want to add any
# config that is in the depend list of another config. This script does
# not enable configs that are not already enabled. If we come across a
# config A that depends on !B, we can still add B to the list of depends
# to keep on. If A was on in the original config, B would not have been
# and B would not be turned on by this script.
#
sub parse_config_dep_select
{
    my ($p) = @_;

    while ($p =~ /[$valid]/) {

	if ($p =~ /^[^$valid]*([$valid]+)/) {
	    my $conf = "CONFIG_" . $1;

	    $p =~ s/^[^$valid]*[$valid]+//;

	    if (!defined($configs{$conf})) {
		# We must make sure that this config has its
		# dependencies met.
		$repeat = 1; # do again
		$configs{$conf} = 1;
	    }
	} else {
	    die "this should never happen";
	}
    }
}

while ($repeat) {
    $repeat = 0;

    foreach my $config (keys %configs) {
	$config =~ s/^CONFIG_//;

	if (!defined($depends{$config})) {
	    next;
	}

	# This config has dependencies. Make sure they are also included
	parse_config_dep_select $depends{$config};

	if (defined($prompt{$config}) || !defined($selects{$config})) {
	    next;
	}

	# config has no prompt and must be selected.
	parse_config_dep_select $selects{$config};
    }
}

my %setconfigs;

# Finally, read the .config file and turn off any module enabled that
# we could not find a reason to keep enabled.
while(<CIN>) {
	if (/^(CONFIG.*)=m/) {
		if (defined($configs{$1})) {
		    $setconfigs{$1} = 1;
		    print;
		} else {
		    print "# $1 is not set\n";
		}
	} else {
		print;
	}
}
close(CIN);

# Integrity check, make sure all modules that we want enabled do
# indeed have their configs set.
loop:
foreach my $module (keys(%modules)) {
    if (defined($objects{$module})) {
	my @arr = @{$objects{$module}};
	foreach my $conf (@arr) {
	    if (defined($setconfigs{$conf})) {
		next loop;
	    }
	}
	print STDERR "module $module did not have configs";
	foreach my $conf (@arr) {
	    print STDERR " " , $conf;
	}
	print STDERR "\n";
    }
}
