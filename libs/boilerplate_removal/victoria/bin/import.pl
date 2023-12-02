#!/usr/bin/perl -W

# Process all group files, download all url and preprocess files
#
# Miroslav Spousta <spousta@ufal.mff.cuni.cz>
# $Id: import.pl 111 2006-05-26 10:04:01Z qiq $

use strict;
use Getopt::Long;
use Digest::SHA1 qw(sha1_hex);
use Data::Dumper;
use Cwd qw(realpath);
use File::Basename;

$import::group_dir = undef;

use constant VERSION => '$LastChangedRevision: 111 $';

# debugging (set DEBUG environment variable to enable)
$import::DEBUG = 0;
$import::verbose = 0;
$import::reload = 0;
$import::force = 0;

sub usage()
{
	my ($retval) = @_;

	print STDERR <<EOT
usage: import.pl [options] [group]
HTML markup tool.

options:
	--reload\tReload invalid urls
	--force\t\tForce reload of all urls
	--verbose\tPrint messages about what we are going to do
	--help\t\tPrint options summary
	--version\tPrint version info and exit

If no files are specified on the command line, input is read from the stdin.
EOT
;
	exit($retval);
}

sub version()
{
	my $ver = VERSION;
	$ver =~ s/\$LastChangedRevision: (.*)[ ]\$/$1/g;
	print "import.pl, version $ver\n";
}

# main()

$import::DEBUG = defined $ENV{"DEBUG"} ? $ENV{"DEBUG"} : 0;
my ($version, $help) = ('', '');
GetOptions(
	'reload' => \$import::reload,
	'force' => \$import::force,
	'verbose' => \$import::verbose,
	'help' => \$help,
	'version' => \$version) || die("Unknown options");

&usage(0) if ($help eq '1');
&version() if ($version eq '1');

# process config file
my $fullpath = realpath(dirname($0));
my $fr;
open($fr, "<$fullpath/../config") or die("Cannot open config file");
while (<$fr>) {
	chomp;
	if (/=/) {
		my @a = split(/\s*=\s*/);
		if ($a[0] eq 'data') {
			$import::data_dir = $a[1];
		} elsif ($a[0] eq 'groups') {
			$import::group_dir = $a[1];
		}
	}
}
(defined $import::data_dir and defined $import::group_dir) or die("Invalid config file");

my @files;
if (@ARGV > 0) {
	while (@ARGV > 0) {
		push(@files, $import::group_dir.'/'.shift(@ARGV).'.group');
	}
} else {
	@files = glob($import::group_dir."/*.group");
}
foreach my $f (@files) {
	print "Group: $1\n" if (@files > 1 and $f =~ /.*\/([^\/]+)\.group$/);
	if (not open($fr, '<'.$f)) {
		print STDERR "Warning: cannot read $f\n";
		next;
	}
	my @lines;
	while (<$fr>) {
		chomp;
		if (/^\s*#/) {
			push(@lines, $_);
			next;
		}
		my @a = split(/\s+/);
		if (@a < 1) {
			print STDERR "Warning: invalid line ($f): $_\n";
			push(@lines, $_);
			next;
		}
		if (@a == 1 or $import::force or ($import::reload and $a[1] ne 'OK')) {
			print STDERR "Loading url: $a[0]..." if ($import::verbose or $import::DEBUG);
			# load given url ($a[0])
			my $dir;
			do {
				$dir = substr(sha1_hex(time), 0, 10);
			} while (not mkdir("$import::data_dir/$dir"));
			# run prepare.sh
			my $fh;
			open($fh, "$fullpath/prepare.sh \"$import::data_dir/$dir\" \"$a[0]\"|") or die("Cannot run prepare.sh");
			my $last = 'ERROR: unknown error';
			my @l;
			while (<$fh>) {
				push(@l, $_);
				$last = $_;
			}
			close($fh);
			chomp($last);
			if ($last =~ s/^OK (.*)/OK $dir\/$1/) {
				system("chmod -R og+rwx \"$import::data_dir/$dir\"");
			}
			push(@lines, "$a[0] $last");
			# save prepare.sh log file
			open($fh, ">$import::data_dir/$dir/.log") or die ("Cannot save log file");
			print $fh join("", @l);
			close($fh);
			print STDERR " $last\n" if ($import::verbose or $import::DEBUG);
		} else {
			print STDERR "skipping: $a[0]\n" if ($import::verbose or $import::DEBUG);
			push(@lines, $_);
		}
	}
	close($fr);
	if (not open($fr, '>'.$f)) {
		print STDERR "Warning: cannot write $f\n";
		next;
	}
	print $fr join("\n", @lines);
	close($fr);
}
