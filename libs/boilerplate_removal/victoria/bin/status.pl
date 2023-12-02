#!/usr/bin/perl -W

# Process all group files, show status of urls
#
# Miroslav Spousta <spousta@ufal.mff.cuni.cz>
# $Id: status.pl 111 2006-05-26 10:04:01Z qiq $

use strict;
use Getopt::Long;
use Digest::SHA1 qw(sha1_hex);
use Data::Dumper;
use Cwd qw(realpath);
use File::Basename;

$status::group_dir = undef;

use constant VERSION => '$LastChangedRevision: 111 $';

# debugging (set DEBUG environment variable to enable)
$status::DEBUG = 0;
$status::verbose = 0;

sub usage()
{
	my ($retval) = @_;

	print STDERR <<EOT
usage: status.pl [options] [group]
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
	print "status.pl, version $ver\n";
}

# main()

$status::DEBUG = defined $ENV{"DEBUG"} ? $ENV{"DEBUG"} : 0;
my ($version, $help) = ('', '');
GetOptions(
	'verbose' => \$status::verbose,
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
			$status::data_dir = $a[1];
		} elsif ($a[0] eq 'groups') {
			$status::group_dir = $a[1];
		}
	}
}
(defined $status::data_dir and defined $status::group_dir) or die("Invalid config file");

my @files;
if (@ARGV > 0) {
	while (@ARGV > 0) {
		push(@files, $status::group_dir.'/'.shift(@ARGV).'.group');
	}
} else {
	@files = glob($status::group_dir."/*.group");
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
		my @a = split(/\s+/, $_, 3);
		if (@a < 1) {
			print STDERR "Warning: invalid line ($f): $_\n";
			push(@lines, $_);
			next;
		}
		if (defined ($a[1]) and $a[1] eq 'OK' and -f "$status::data_dir/$a[2]") {
			if (-f "$status::data_dir/$a[2].xml" and -s "$status::data_dir/$a[2].xml" > 0) {
				my $fr2;
				my %counts;
				open($fr2, "<$status::data_dir/$a[2].xml");
				while (<$fr2>) {
					chomp;
					if ($_ =~ /<(a[^ ]+)/) {
						$counts{$1}++;
					}
				}
				close($fr2);
				my @counts;
				foreach my $k (keys %counts) {
					push(@counts, "$k:$counts{$k}");
				}
				print "$a[0] (".join(", ", @counts).")\n";
			} else {
				print "$a[0] EMPTY\n";
			}
		} else {
			print "$a[0] INVALID";
			print " ($a[2])" if (defined $a[2]);
			print "\n";
		}
	}
	close($fr);
}
