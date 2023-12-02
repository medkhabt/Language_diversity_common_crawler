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
	$path = substr(urldecode($_SERVER['REQUEST_URI']), strlen(preg_replace("/^(.*\/store.php).*/", "\\1", $_SERVER['SCRIPT_NAME']))+1);
	$fullprefix = realpath($data);
	$full = realpath($data.'/'.$path);
	$fullpath = dirname($full);
	if (!$fullpath || !$fullprefix)
		error('File does not exist');
	if (substr($fullpath, 0, strlen($fullprefix)) != $fullprefix)
		error('Invalid path');

	$fw = @fopen("$full.xml", 'w');
	if (!$fw)
		xml_error("Cannot open file for writing");
	fclose($fw);
	foreach ($_FILES as $key => $val) {
        	switch($val['error']) {
        	case UPLOAD_ERR_NO_FILE:
               		/* ok, no file was specified */
			xml_error("File not specified");
			break;
        	case UPLOAD_ERR_OK:
               		if (!is_uploaded_file($val['tmp_name']))
				xml_error("Upload error");
			/* check that there is at least one item,
			   we need this for simple "already annotated"
			   detection */
			$fr = @fopen($val['tmp_name'], 'r');
			$n = 0;
			while (!feof($fr)) {
				$l = fgets($fr, 4096);
				if (!eregi("<[/]?data[ >]", $l))
					$n++;
			}
			fclose($fr);
			if ($n > 0) 
				rename($val['tmp_name'], "$full.xml");
			else
				unlink("$full.xml");
			break;
       		case UPLOAD_ERR_INI_SIZE:
       		case UPLOAD_ERR_FORM_SIZE:
               		xml_error("File too big");
			break;
		default:
               		xml_error("Unknown error");
			break;
		}
		header("HTTP/1.0 200 OK");
		header("Content-Type: text/xml");
		print "<ok />";
	}
	@chmod("$full.xml", 0644);
?>
