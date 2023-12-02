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

	if ($_SESSION['user'] != 'qiq')
		error("Invalid access");
	$data = isset($_GET['data']) ? $_SESSION['data_dir'] : $_SESSION['demo_data_dir'];
	system("rm -rf $data/*");
?>
