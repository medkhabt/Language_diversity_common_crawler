<?php

	include('authorize.php');
	authorize();

	if (!is_file($help)) { ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1.dtd">
<html>
<head>
<title>HTML Annotation: Help</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
</head>
<body>
<div>Help not available.</div>
</body>
</html>
<?php	}
	readfile($help);
?>
