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

	$data = $_SESSION['user'] == 'demo' ? $_SESSION['demo_data_dir'] : $_SESSION['data_dir'];
	$base_proto = isset($_SERVER['HTTPS']) ? 'https' : 'http';
	$base_host = $_SERVER['SERVER_NAME'];
	$base_port = ':'.$_SERVER['SERVER_PORT'];
	if (($base_proto == 'http' && $base_port == ':80') || ($base_proto == 'https' && $base_port == ':443'))
		$base_port = '';
	$base_path = preg_replace("/(.*?)\/proxy.php.*/", "\\1", $_SERVER['SCRIPT_NAME']);
	$base = "$base_proto://$base_host$base_port$base_path";

	//$prefix = preg_replace("^(.*?)/proxy.php.*", "\\1", $_SERVER['SCRIPT_FILENAME']);
	# UGH: not working: $path = substr($_SERVER['REQUEST_URI'], strlen($_SERVER['SCRIPT_NAME'])+1);
	$path = substr(urldecode($_SERVER['REQUEST_URI']), strlen(preg_replace("/^(.*?\/proxy.php).*/", "\\1", $_SERVER['SCRIPT_NAME']))+1);
	$fullprefix = realpath($data);
	$full = realpath($data.'/'.$path);
	$fullpath = dirname($full);
	if (!$fullpath || !$fullprefix)
		error('File does not exist');
	if (substr($fullpath, 0, strlen($fullprefix)) != $fullprefix)
		error('Invalid path');
	$dataprefix = ereg_replace("^([^/]+/[^/]+)/.*", "\\1", $path);

	$dn = dirname($full);
	$fn = basename($full);
	$frh = @fopen("$dn/.pavuk_info/$fn", 'r');
	$content_type = "text/html; charset=utf-8";
	if ($frh) {
		$redirect = 0;
		$location = "";
		while (!feof($frh)) {
			$buffer = fgets($frh, 4096);
			if (eregi("^HTTP/[01]\.[01] (3[0-9]{2})", $buffer, $i)) {
				$redirect = 1;
				$code = $i[1];
			} elseif (eregi("^Content-Type: ([[:alnum:]/-]*)", $buffer, $i)) {
				$content_type = $i[1];
				if ($content_type == 'text/html')
					$content_type = "text/html; charset=utf-8";
			} elseif ($redirect && eregi("^Location: ([^ ]+)", $buffer, $i)) {
				$location = $i[1];
			}
		}
		fclose($frh);

		if ($redirect && !empty($location)) {
			if (ereg("^(https?)://([^/:]+)(:[0-9]+)?/(.*)", $location, $i)) {
				$proto = $i[1]; $host = $i[2]; $port = $i[3]; $p = $i[4];
			} elseif (ereg("^/(.*)", $location, $i)) {
				$p = $i[1];
				ereg("^[^/]+/[^/]+/([^/]+)/([^_]+)_([^/]+)", $path, $i);
				$proto = $i[1]; $host = $i[2]; $port = $i[3];
			} else {
				ereg("^[^/]+/[^/]+/([^/]+)/([^_]+)_([^/]+)/(.*/)[^/]+$", $path, $i);
				$proto = $i[1]; $host = $i[2]; $port = $i[3]; $p = $i[4].$location;
			}
			if (empty($port))
				$port = $proto == 'http' ? 80 : 443;
			$location = "$base/proxy.php/$dataprefix/$proto/${host}_$port/$p";
			header("Location: $location", $code);
			exit;
		}
	}

	header("HTTP/1.0 200 OK");
	header("Content-Type: $content_type");
	$fr = fopen($full, 'r');
	if (!$fr)
		error('Cannot open file');
	if ($content_type == 'text/html; charset=utf-8') {
		while (!feof($fr)) {
			$buffer = fgets($fr, 4096);
			if (eregi("(.*)</head>(.*)", $buffer, $index)) {
				print $index[1]; 
				global $bg_colors, $fg_colors, $overlap, $buttons, $buttons2; ?>
	<script type="text/javascript" src="<?php print $base; ?>/annotation.js"></script>
	<script type="text/javascript">
		var bg = new Array(<?php print implode(", ", $bg_colors); ?>);
		var fg = new Array(<?php print implode(", ", $fg_colors); ?>);
		var overlap = <?php print $overlap; ?>;
		var buttons = <?php print count($buttons); ?>;
		var buttons2 = Array(
<?php			for ($i = 0; $i < count($buttons); $i++) {
				if (isset($buttons2[$i])) { ?>
			Array(<?php print "'".implode("', '", $buttons2[$i])."'"; ?>)<?php print ($i == count($buttons)-1) ? '' : ','; ?>
<?php				} else { ?>
			null<?php print ($i == count($buttons)-1) ? '' : ','; ?>
<?php				}
			} ?>
		);
	</script>
	<link rel="stylesheet" type="text/css" href="<?php print $base; ?>/annotation.css" />
</head>
<?php				print $index[2];
			} elseif (eregi("(.*)<body([^>]*)>(.*)", $buffer, $index)) {
				print $index[1]; ?>
<body<?php print $index[2]; ?> onload="annotation_register('<?php print $base; ?>', '<?php print $path; ?>');" onkeypress="annotation_keyPress(event);">
	<div id="annotation_box"><span>Loading...</span></div>
	<div id="annotation_trans"><span>&nbsp;</span></div>
<?php				print $index[3];
			} else {
				print $buffer;
			}
		}
	} else {
		header("Content-Length: ".filesize($full));
		fpassthru($fr);
	}
	fclose($fr);
?>
