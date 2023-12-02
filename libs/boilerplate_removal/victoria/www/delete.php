<?php
	include("authorize.php");
	authorize();

	$path = $_SESSION['demo_data_dir'];
	system("rm -rf \"$path/\"*");
?>
