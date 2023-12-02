#!/usr/bin/perl -W

# Simple HTML markup tool.
#
# Miroslav Spousta <spousta@ufal.mff.cuni.cz>
# $Id: htmlmark.pl 111 2006-05-26 10:04:01Z qiq $

use HTML::TreeBuilder;
use HTML::Element;
use HTML::Entities;
use XML::LibXML;
use Getopt::Long;
use Data::Dumper;
use strict;

use constant VERSION => '$LastChangedRevision: 111 $';

# debugging (set DEBUG environment variable to enable)
$htmlmark::DEBUG = 0;

$htmlmark::tag = 'span';

%htmlmark::text_element = ();
%htmlmark::ignore_element = ();
$htmlmark::fout = undef;
$htmlmark::id = 0;
$htmlmark::in_body = 0;
$htmlmark::in_script = 0;
$htmlmark::in_iframe = 0;
$htmlmark::in_noscript = 0;
@htmlmark::tags = ();
$htmlmark::merge = 0;

%htmlmark::header = ();
%htmlmark::text = ();
%htmlmark::other = ();

%htmlmark::disabled_attr = (
	'onblur' => 1, 'onchange' => 1, 'onclick' => 1, 'ondblclick' => 1,
	'onfocus' => 1, 'onkeydown' => 1, 'onkeypress' => 1, 'onkeyup' => 1,
	'onload' => 1, 'onmousedown' => 1, 'onmousemove' => 1, 'onmouseout' => 1,
	'onmouseover' => 1, 'onmouseup' => 1, 'onreset' => 1, 'onselect' => 1,
	'onsubmit' => 1, 'onunload' => 1
);

%htmlmark::text_element = (
	'strike' => 1, 'form' => 1, 'h5' => 1, 'caption' => 1, 'code' => 1,
	'acronym' => 1, 'strong' => 1, 'h4' => 1, 'em' => 1, 'b' => 1,
	'q' => 1, 'applet' => 1, 'span' => 1, 'title' => 1, 'small' => 1,
	'body' => 1, 'var' => 1, 'del' => 1, 'blockquote' => 1, 'dfn' => 1,
	'iframe' => 1, 'h3' => 1, 'a' => 1, 'tt' => 1, 'font' => 1,
	'noframes' => 1, 'u' => 1, 'abbr' => 1, 'sup' => 1, 'h6' => 1,
	'address' => 1, 'th' => 1, 'h1' => 1, 'legend' => 1, 'dd' => 1,
	's' => 1, 'li' => 1, 'td' => 1, 'label' => 1, 'kbd' => 1,
	'object' => 1, 'div' => 1, 'dt' => 1, 'pre' => 1, 'center' => 1,
	'samp' => 1, 'cite' => 1, 'i' => 1, 'bdo' => 1, 'h2' => 1, 'ins' => 1,
	'p' => 1, 'fieldset' => 1, 'sub' => 1, 'big' => 1, 'button' => 1,
	'noscript' => 1
);

# all no-ending tag elements must be ignored

%htmlmark::ignore_element = (
	'link' => 1, 'base' => 1, 'script' => 1, 'br' => 1, 'input' => 1,
	'area' => 1, 'style' => 1, 'img' => 1, 'meta' => 1, 'isindex' => 1,
	'hr' => 1, 'col' => 1, 'basefont' => 1, 'param' => 1
);

sub usage()
{
	my ($retval) = @_;

	print STDERR <<EOT
usage: htmlmark.pl [options] [--merge file.xml] [input...]
HTML markup tool.

options:
	--merge\t\tMerge annotation from the XML file
	--output=file\tOutput XML file (stdout)
	--tag=string\tTag to be used for annotation markup ($htmlmark::tag)
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
	print "htmlmark.pl, version $ver\n";
}

# process XML file with saved annotation

sub process_annotation()
{
	my ($root) = @_;

	# process elements
	foreach my $element ($root->getElementsByTagName('at1')) {
		my $start = $element->getAttribute('start');
		my $end = $element->getAttribute('end');
		for (my $i = $start; $i <= $end; $i++) {
			$htmlmark::header{$i} = 1;
		}
	}
	foreach my $element ($root->getElementsByTagName('at2')) {
		my $start = $element->getAttribute('start');
		my $end = $element->getAttribute('end');
		for (my $i = $start; $i <= $end; $i++) {
			$htmlmark::text{$i} = 1;
		}
	}
	foreach my $element ($root->getElementsByTagName('at3')) {
		my $start = $element->getAttribute('start');
		my $end = $element->getAttribute('end');
		for (my $i = $start; $i <= $end; $i++) {
			$htmlmark::other{$i} = 1;
		}
	}
}

# HTML parser

sub html_text()
{
	my ($x, $text, $skipped) = @_;
	$text = $skipped.$text;
	if (not $htmlmark::in_body) {
		print $htmlmark::fout $text;
		return;
	}
	if ($text =~ /^\s*$/) {
		print $htmlmark::fout $text;
		return;
	}
		
	my $toplevel = $#htmlmark::tags >= 0 ? $htmlmark::tags[$#htmlmark::tags] : undef;
	if (defined $toplevel and defined $htmlmark::text_element{$toplevel}) {
		if (not $htmlmark::merge) {
			printf $htmlmark::fout "<%s id=\"at%d\">%s</%s>", $htmlmark::tag, $htmlmark::id++, $text, $htmlmark::tag;
		} else {
			my $id = $htmlmark::id++;
			if (defined($htmlmark::header{$id})) {
				print $htmlmark::fout "<span id=\"victor_an$id\" class=\"victor_header\">$text</span>";
			} elsif (defined($htmlmark::text{$id})) {
				print $htmlmark::fout "<span id=\"victor_an$id\" class=\"victor_text\">$text</span>";
			} elsif (defined($htmlmark::other{$id})) {
				print $htmlmark::fout "<span id=\"victor_an$id\" class=\"victor_other\">$text</span>";
			} else {
				#print $htmlmark::fout $text;
				print $htmlmark::fout "<span id=\"victor_an$id\" class=\"victor_other\">$text</span>";
			}
		}
	} else {
		print $htmlmark::fout $text;
	}
}

sub reset_handlers()
{
	my ($html) = @_;

	$html->handler(text => undef);
	$html->handler(comment => undef);
	$html->handler(declaration => undef);
	$html->handler(start => undef);
}

sub set_handlers()
{
	my ($html) = @_;

	$html->handler(text => \&html_text, 'self, text, skipped_text');
	$html->handler(comment => \&html_comment, 'self, text');
	$html->handler(declaration => \&html_declaration, 'self, text');	
	$html->handler(start => \&html_start, 'self, tagname, attr, attrseq, text');
}

sub html_start()
{
	my ($html, $tag, $attr, $attrseq, $text) = @_;

	# lowercase everything;
	$tag = lc($tag);
	my %a;
	foreach my $k (keys %{$attr}) {
		$a{lc($k)} = $$attr{$k};
	}

	# body
	if ($tag eq 'body') {
		$htmlmark::in_body = 1;
	}

	if ($tag eq 'script') {
		&reset_handlers($html);
		$htmlmark::in_script = 1;
		print $htmlmark::fout $text if ($htmlmark::merge);
		return;
	} elsif ($tag eq 'iframe') {
		&reset_handlers($html);
		$htmlmark::in_iframe = 1;
		print $htmlmark::fout $text if ($htmlmark::merge);
		return;
	} elsif ($tag eq 'noscript') {
		&reset_handlers($html);
		$htmlmark::in_noscript = 1;
		print $htmlmark::fout $text if ($htmlmark::merge);
		return;
	} elsif ($tag eq 'noframes') {
		&reset_handlers($html);
		$htmlmark::in_noframes = 1;
		print $htmlmark::fout $text if ($htmlmark::merge);
		return;
	} elsif ($tag eq 'style') {
		&reset_handlers($html);
	}
	# merge -> do almost nothing
	if ($htmlmark::merge) {
		print $htmlmark::fout $text;
	} else {
		print $htmlmark::fout "<$tag";
		foreach my $k (keys %a) {
			next if (defined $htmlmark::disabled_attr{$k});
			next if ($tag eq 'a' and $k eq 'target');
			my $val = $a{$k};
			if ($k eq 'href' or $k eq 'src') {
				$val =~ s/\?/%3F/g;
				$val =~ s/&([^a])/%26$1/g;
			}
			$val = 'javascript:void(0);' if (($tag eq 'a' or $tag eq 'area') and $k eq 'href');
			$val = 'javascript:void(0);' if ($tag eq 'form' and $k eq 'action');
			$val = '_'.$val if ($k eq 'id' and ($val =~ /^a[to][0-9]+/));
			printf $htmlmark::fout " %s=\"%s\"", $k, encode_entities($val, "<>&\"'");
		}
		print $htmlmark::fout " /" if ($text =~ /\/>$/);
		print $htmlmark::fout ">";
	}
	push(@htmlmark::tags, $tag) if (not defined $htmlmark::elements_ignore{$tag});
}

sub html_end()
{
	my ($html, $tag, $text, $skipped) = @_;

	$tag = lc($tag);

	# body
	if ($tag eq 'body') {
		$htmlmark::in_body = 0;
	}

	if ($htmlmark::in_script) {
		if ($tag eq 'script') {
			&set_handlers($html);
			$htmlmark::in_script = 0;
		}
		print $htmlmark::fout $skipped.$text if ($htmlmark::merge);
		return;
	} elsif ($htmlmark::in_iframe) {
		if ($tag eq 'iframe') {
			&set_handlers($html);
			$htmlmark::in_iframe = 0;
		}
		print $htmlmark::fout $skipped.$text if ($htmlmark::merge);
		return;
	} elsif ($htmlmark::in_noscript) {
		if ($tag eq 'noscript') {
			&set_handlers($html);
			$htmlmark::in_noscript = 0;
		}
		print $htmlmark::fout $skipped.$text if ($htmlmark::merge);
		return;
	} elsif ($htmlmark::in_noframes) {
		if ($tag eq 'noframes') {
			&set_handlers($html);
			$htmlmark::in_noframes = 0;
		}
		print $htmlmark::fout $skipped.$text if ($htmlmark::merge);
		return;
	} elsif ($tag eq 'style') {
		&set_handlers($html);
		#$skipped =~ s/(\@import "[^"]*)\?/$1%3F/g if (not $htmlmark::merge);
		print $htmlmark::fout $skipped;
	}
	print $htmlmark::fout $text;
	if (not defined $htmlmark::elements_ignore{$tag}) {
		while (@htmlmark::tags > 0 and pop(@htmlmark::tags) ne $tag) { }
	}
}

sub html_declaration()
{
	my ($x, $text) = @_;
	print $htmlmark::fout $text;
}

sub html_comment()
{
	my ($x, $text) = @_;
	print $htmlmark::fout $text;
}

sub parse_file()
{
	my ($filename) = @_;
	my $fin;

	if (defined $filename) {
		print STDERR "Parsing ($filename)\n" if ($htmlmark::verbose or $htmlmark::DEBUG);
		if (not open($fin, "<".$filename)) {
			print STDERR "Cannot open file $filename\n";
			$htmlmark::error = 1;
			return undef;
		}
	} else {
		print STDERR "Parsing (stdin)\n" if ($htmlmark::verbose or $htmlmark::DEBUG);
		$fin = \*STDIN;
	}
	binmode $fin, ":utf8";
	my $html = HTML::Parser->new(api_version => 3);
	$html->handler(end => \&html_end, 'self, tagname, text, skipped_text');
	&set_handlers($html);
	$html->xml_mode(1);
	$html->marked_sections(1);
	$html->unbroken_text(1);
	$html->parse_file($fin);
	$html->eof();
	close($fin);
}

sub process_files()
{
	my ($output, $input) = @_;
	my ($fin, $fout);

	# open output file
	if ($output ne '') {
		open($fout, ">:utf8", $output) or die("Cannot open file $output");
	} else {
		$fout = \*STDOUT;
		binmode $fout, ":utf8";
	}
	$htmlmark::fout= $fout;

	# process input file(s)
	if (@{$input} > 0) {
		foreach my $filename (@{$input}) {
			&parse_file($filename);
		}
	} else {
		&parse_file(undef);
	}

	close($fout);
}

# main()

$htmlmark::DEBUG = defined $ENV{"DEBUG"} ? $ENV{"DEBUG"} : 0;

# command line processing
my ($output, $version, $help) = ('', '', '');
GetOptions(
	'merge' => \$htmlmark::merge,
	'output=s' => \$output,
	'tag=s' => \$htmlmark::tag,
	'verbose' => \$htmlmark::verbose,
	'help' => \$help,
	'version' => \$version) || die("Unknown options");
&usage(0) if ($help eq '1');
&version() if ($version eq '1');

if ($htmlmark::merge eq '1' and @ARGV < 2) {
	print STDERR "Result xml file must be specified!\n";
	&usage();
	exit(1);
}

# Result (XML) preprocessing
if ($htmlmark::merge) {
	my $result_file = shift @ARGV;
	my $parser = XML::LibXML->new();
	$parser->keep_blanks(0);
	my $tree = $parser->parse_file($result_file);
	die("Cannot open/read/parse result XML file") if (not defined $tree);
	my $root = $tree->getDocumentElement;
	&process_annotation($root);
}

binmode \*STDERR, ":utf8";

# process HTML file(s)
$htmlmark::error = 0;
&process_files($output, \@ARGV);

exit($htmlmark::error);
