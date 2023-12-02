<?php

define("config_file", "../config");

function process_config()
{
	global $data_dir, $demo_data_dir, $group_dir, $user_file, $buttons, $buttons2;
	global $bg_colors, $fg_colors, $overlap, $help;
	$fh = fopen(config_file, "r");
	if (!$fh) {
		print "Cannot open config file\n";
		exit;
	}
	$f = null;
	while (!feof($fh)) {
		$l = fgets($fh, 4096);
		$l = chop($l);
		if (!ereg("^[ \t]*[[:alnum:]]", $l))
			continue;
		$f = split("[ \t]*=[ \t]*", $l);
		switch ($f[0]) {
		case 'data':
			$data_dir = $f[1];
			break;
		case 'demodata':
			$demo_data_dir = $f[1];
			break;
		case 'groups':
			$group_dir = $f[1];
			break;
		case 'users':
			$user_file = $f[1];
			break;
		case 'buttons':
			if (!empty($f[1])) {
				$buttons = explode(",", $f[1]);
				$buttons2 = array();
				for ($i = 0; $i < count($buttons); $i++) {
					if (preg_match("/(.*)\((.*)\)/", $buttons[$i], $index)) {
						$buttons[$i] = $index[1];
						$buttons2[$i] = explode(":", $index[2]);
					}
				}
			} else {
				$buttons = array();
				$buttons2 = array();
			}
			break;
		case 'bgcolors':
			$bg_colors = explode(",", $f[1]);
			break;
		case 'fgcolors':
			$fg_colors = explode(",", $f[1]);
			break;
		case 'overlap':
			$overlap = $f[1];
			break;
		case 'help':
			$help = $f[1];
			break;
		default:
			print "Unknown config line: $l\n";
		}
	}
	fclose($fh);
	if (!isset($data_dir) || !isset($demo_data_dir) || !isset($group_dir) || !isset($user_file) || !isset($buttons)
	   || !isset($bg_colors) || !isset($fg_colors) || !isset($overlap) || !isset($help))
		die("Incomplete config file!");
}

function get_user($username)
{
	global $user_file;
	if (!isset($user_file))
		process_config();
	$fh = fopen($user_file, "r");
	if (!$fh) {
		print "Cannot open authorization file\n";
		exit;
	}
	$f = null;
	while (!feof($fh)) {
		$l = fgets($fh, 4096);
		$l = chop($l);
		if (!ereg("^[[:alnum:]]", $l))
			continue;
		$f = split("[ \t]+", $l);
		if ($f[0] == $username)
			break;
	}
	fclose($fh);
	return $f;
}

function authorize($redirect = 1)
{
        //ini_set('session.cookie_lifetime', 2*3600); /* 0 */
        ini_set('session.cookie_lifetime', 0);
        ini_set('session.use_only_cookies', 1);
        ini_set('memory_limit', '20M');

        /* do not time-out */
        set_time_limit(0);
        session_start();

	$authorized = isset($_SESSION['user']);
	if ($redirect && !$authorized) {
		session_write_close();
		header('Location: '.new_url('login.php'), 303);
		exit;
	}
	process_config();
	return $authorized;
}

function deauthorize()
{
	session_start();
	$_SESSION = array();
	//if (isset($_COOKIE[session_name()]))
	//        setcookie(session_name(), '', time()-42000, '/');
	session_destroy();
}

function new_url($new)
{
        if ($_SERVER['SERVER_PORT'] == 443) {
                $method = "https://";
        } else {
                $method = "http://";
        }
        if (isset($new)) {
                $path = dirname($_SERVER['PHP_SELF'])."/".$new;
        } else {
                $path = $_SERVER['PHP_SELF'];
        }
        $path = ereg_replace("/[^/]+/\.\./", "/", $path);
        return $method.$_SERVER['HTTP_HOST'].$path;
}

function redirect($page)
{
	header('Location: '.new_url($page), 303);
}
?>
