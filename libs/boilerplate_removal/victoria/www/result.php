<?php
	include("authorize.php");
	authorize();

	function error($text)
	{
		header("HTTP/1.0 404 Error");
		header("Content-Type: text/html");
		print $text;
		exit;
	}

	$data = $_SESSION['demo_data_dir'];
	# UGH: not working: $path = substr($_SERVER['REQUEST_URI'], strlen($_SERVER['SCRIPT_NAME'])+1);
	$path = substr(urldecode($_SERVER['REQUEST_URI']), strlen(preg_replace("/^(.*?\/result.php).*/", "\\1", $_SERVER['SCRIPT_NAME']))+1);
	$fullprefix = realpath($data);
	$full = realpath($data.'/'.$path);
	$fullpath = dirname($full);
	if (!$fullpath || !$fullprefix)
		error('File does not exist');
	if (substr($fullpath, 0, strlen($fullprefix)) != $fullprefix)
		error('Invalid path');

	if (is_file("$full.xml"))
		passthru("PERL5LIB=/home/spousta/perl/lib/perl5/site_perl ../bin/htmlmark.pl --merge \"$full.xml\" \"$full.in\" 2>&1");
	else
		passthru("cat \"$full.in\"");
?>
