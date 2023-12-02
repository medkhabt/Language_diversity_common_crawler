<?php

function setup_session()
{
	global $data_dir, $demo_data_dir, $group_dir, $user_file;
	if (!isset($data_dir))
		process_config();
	$_SESSION['data_dir'] = $data_dir;
	$_SESSION['demo_data_dir'] = $demo_data_dir;
	$_SESSION['group_dir'] = $group_dir;
	$_SESSION['user_file'] = $user_file;
	session_write_close();
	redirect('index.php');
}

	include('authorize.php');

	if (isset($_GET['demo'])) {
		authorize(0);
		if (empty($_SESSION['demo_data_dir']))
			redirect('login.php');
		$_SESSION['user'] = 'demo';
		setup_session();
		exit;
	}
	if (isset($_POST['username']) && isset($_POST['password'])) {
		$username = $_POST['username'];
		$password = $_POST['password'];
		$f = get_user($username);
		if (isset($f) && count($f) >= 2 && ($f[1] == sha1($password) || $f[1] == $password)) {
			authorize(0);
			$_SESSION['user'] = $f[0];
			setup_session();
			exit;
		}
	}
	process_config();
	deauthorize();
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1.dtd">
<html>
<head>
<title>HTML Annotation: Login</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<style type="text/css">
body {
	width: 100%;
	margin: 0;
	background: #E6ECFF;
}

form {
	font: 0.8em sans-serif;
        top: 50%;
        left: 50%;
	height: 6em;
	width: 18em;
        margin-top: -25px;
        margin-left: -75px;
        position: absolute;
}

form table {
	border: 1px solid black;
}

form input {
	border: 1px solid black;
}

form td {
	text-align: right;
}

form div {
	text-align: center;
}

#button
{
	text-align: center;
}
</style>
</head>
<body id="mainbody" onload="document.getElementById('username').focus();">
<form id="loginform" action="login.php" method="post" enctype="multipart/form-data">
<table>
<tr><td>Username:</td><td><input type="text" id="username" name="username"/></td></tr>
<tr><td>Password:</td><td><input type="password" name="password"/></td></tr>
<tr><td colspan="2" id="button"><input type="submit" name="login" value="Login"/></td></tr>
</table>
<?php	global $demo_data_dir;
	if (!empty($demo_data_dir)) { ?>
<div><a href="login.php?demo">Demo access</a></div>
<?php	} ?>
</form>

<!--<div id="clear"></div>-->
</body>
</html>
