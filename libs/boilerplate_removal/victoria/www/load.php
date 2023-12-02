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

	/* load.php is used only in demo mode */
	$data = $_SESSION['demo_data_dir'];
	if (empty($data) || !is_dir($data))
		error("Invalid demo directory");
	if (!ereg("^(f)?url=(.*)", $_SERVER['QUERY_STRING'], $i))
		exit;
	$force = $i[1] == 'f' ? 1 : 0;
	$url = $i[2];
	do {
		$dir = substr(sha1(time()), 0, 10);
	} while (!@mkdir("$data/$dir", 0777));
?>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>empty</title>
<style type="text/css">
#annotation_box {
	width: 148px;
	border: 1px solid #000;
	height: 48px;
	background: white;
	text-align: center;
	line-height: 48px;
	z-index: 2;
	top: 50%;
	left: 50%;
	margin-top: -25px;
	margin-left: -75px;
	position: fixed;
}

#annotation_trans {
	width: 100%;
	height: 100%;
	position: fixed;
	left: 0;
	top: 0;
	z-index: 1;
	filter: alpha(opacity=60);
	opacity: 0.6;
	background: #CCCCCC;
}
</style>
<script type="text/javascript">
var file = null;

function loadingDone()
{
	if (error != '') {
		alert(error);
		hide_info_box();
	} else {
		location.href = 'proxy.php/'+file;
	}
}
</script>
<script type="text/javascript" src="annotation.js"></script>
</head>
<body onload="loadingDone();">
<div id="annotation_box"><span>Processing...</span></div><div id="annotation_trans">&nbsp;</div>
<pre>
<?php
	$file = system("../bin/prepare.sh \"$data/$dir\" \"$url\" 2>&1");
	if (substr($file, 0, 3) == 'OK ') {
		$file = substr($file, 3);
		print "<script>file = '$dir/$file'; error = '';</script>\n";
	} else {
		print "<script>file = ''; error = '$file';</script>\n";
	}
?>
</pre>
</body>
</html>
