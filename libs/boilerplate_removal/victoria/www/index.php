<?php
	include("authorize.php");
	authorize();
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Frameset//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-2"/>
<title>HTML Annotation</title>
</head>
<frameset rows="30,*" border="0">
  <frame src="index_top.php" name="_top" scrolling="no" />
  <frame src="empty.html" name="_main" />
  <noframes>
    <body>
      <p>Sorry, frame support is necessary.</p>
    </body>
  </noframes>
</frameset>
</html>
