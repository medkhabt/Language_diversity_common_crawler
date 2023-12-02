#!/usr/bin/perl -W

# Process all group files, merge annotation
#
# Miroslav Spousta <spousta@ufal.mff.cuni.cz>
# $Id: export.pl 111 2006-05-26 10:04:01Z qiq $

use strict;
use Getopt::Long;
use Digest::SHA1 qw(sha1_hex);
use Data::Dumper;
use Cwd qw(realpath);
use File::Basename;

$export::group_dir = undef;

use constant VERSION => '$LastChangedRevision: 111 $';

# debugging (set DEBUG environment variable to enable)
$export::DEBUG = 0;
$export::verbose = 0;

sub usage()
{
	my ($retval) = @_;

	print STDERR <<EOT
usage: export.pl [options] [group]
HTML markup tool.

options:
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
	print "export.pl, version $ver\n";
}

# main()

$export::DEBUG = defined $ENV{"DEBUG"} ? $ENV{"DEBUG"} : 0;
my ($version, $help) = ('', '');
GetOptions(
	'verbose' => \$export::verbose,
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
			$export::data_dir = $a[1];
		} elsif ($a[0] eq 'groups') {
			$export::group_dir = $a[1];
		}
	}
}
(defined $export::data_dir and defined $export::group_dir) or die("Invalid config file");

my @files;
if (@ARGV > 0) {
	while (@ARGV > 0) {
		push(@files, $export::group_dir.'/'.shift(@ARGV).'.group');
	}
} else {
	@files = glob($export::group_dir."/*.group");
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
		if (defined($a[1]) and $a[1] eq 'OK' and -f "$export::data_dir/$a[2].in") {
			if (-f "$export::data_dir/$a[2].xml" and -s "$export::data_dir/$a[2].xml") {
				# merge .in and .xml
				system("./htmlmark.pl --merge \"$export::data_dir/$a[2].xml\" --output \"$export::data_dir/$a[2].out\" \"$export::data_dir/$a[2].in\"");
			} else {
				system("cp \"$export::data_dir/$a[2].in\" \"$export::data_dir/$a[2].out\"");
			}
			print "$export::data_dir/$a[2].out\n";
		} else {
			print STDERR "skipping: $a[0]\n" if ($export::verbose or $export::DEBUG);
			push(@lines, $_);
		}
	}
	close($fr);
}
