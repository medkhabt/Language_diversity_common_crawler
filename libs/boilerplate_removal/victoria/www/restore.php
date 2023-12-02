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

	function xml_error($text)
	{
		header("HTTP/1.0 403 Error");
		header("Content-Type: text/xml");
		print "<error text=\"$text\" />";
		exit;
	}

	$data = $_SESSION['user'] == 'demo' ? $_SESSION['demo_data_dir'] : $_SESSION['data_dir'];
	# UGH: not working: $path = substr($_SERVER['REQUEST_URI'], strlen($_SERVER['SCRIPT_NAME'])+1);
	$path = substr(urldecode($_SERVER['REQUEST_URI']), strlen(preg_replace("/^(.*?\/restore.php).*/", "\\1", $_SERVER['SCRIPT_NAME']))+1);
	$fullprefix = realpath($data);
	$full = realpath($data.'/'.$path);
	$fullpath = dirname($full);
	if (!$fullpath || !$fullprefix)
		error('File does not exist');
	if (substr($fullpath, 0, strlen($fullprefix)) != $fullprefix)
		error('Invalid path');

	$fr = @fopen("$full.xml", 'r');
	if (!$fr) {
		$fr = @fopen("$full.xml", 'w+');
		if (!$fr)
			xml_error('Cannot open file for writing');
	}
	header("HTTP/1.0 200 OK");
	header("Content-Type: text/xml");
	while ($data = fread($fr, 1024))
		print $data;
	fclose($fr);
?>
