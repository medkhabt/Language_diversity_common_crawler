<?php
	include("authorize.php");
	authorize();
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1.dtd">
<html>
<head>
<title>HTML Annotation</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<style type="text/css">
body {
	width: 100%;
	margin: 0;
	background: #E6ECFF;
}

form {
	font: 0.8em sans-serif;
	padding-top: 2px;
	padding-bottom: 2px;
}

form input {
	border: 1px solid;
}

form div {
	float: left;
	padding: 2px 10px 2px 10px;
}

<?php	global $buttons, $bg_color, $fg_color;
	for ($i = 1; $i <= count($buttons); $i++) {
		if ($fg_colors != 'null')
			$fg = ereg_replace("'(.*)'", "\\1", $fg_colors[$i+1]);
		else
			$fg = 'black';
		if ($bg_colors != 'null')
			$bg = ereg_replace("'(.*)'", "\\1", $bg_colors[$i+1]);
		else
			$bg = 'white'; ?>
#b<?php print $i; ?> {
	background: <?php print $bg; ?>;
	color: <?php print $fg; ?>;
	border-color: <?php if ($i == 1) print "#000000"; else print "#CCCCCC"; ?>;
}
<?php	} ?>

#url {
	width: 300px;
}

#urlbar {
	background: #DDDDDD;
	padding-left: 10px;
}

#urlbar select {
	border-width: 0;
/*	font: 0.8em sans-serif;*/
}

#resultbar {
	float: left;
	/*padding-top: 3px;*/
}

#helpbar {
	float: right;
	padding-top: 3px;
}

#clear {
	clear: both;
}

</style>
<script type="text/javascript">

var active = false;
var active_button = 2;
<?php
print "var user = '".$_SESSION['user']."';\n";
?>

var active_page_path = null;

function getSelected(select)
{
	var e = select.firstChild;
	while (e != undefined) {
		if (e.nodeName == "OPTION") {
			if (e.selected)
				return e.value;
		}
		e = e.nextSibling;
	}
	return null;
}

function getSelectedNext(select)
{
	var e = select.firstChild;
	var sel = 0;
	while (e != undefined) {
		if (e.nodeName == "OPTION") {
			if (e.selected) {
				sel = 1;
			} else if (sel) {
				return e.value;
			}
		}
		e = e.nextSibling;
	}
	return null;
}

function change_button(key)
{
	switch (key) {
<?php	global $buttons;
	for ($i = 1; $i <= count($buttons); $i++) {
		$k = strtolower(substr($buttons[$i-1], 0, 1)); ?>
	case <?php print $i; ?>:
	case '<?php print $i; ?>':
	case '<?php print $k; ?>':
		active_button = <?php print $i; ?>;
		break;
<?php	} ?>
	}
	for (var i = 1; i <= <?php print count($buttons); ?>; i++) {
		var id = document.getElementById('b'+i);
		if (i == active_button) {
			id.style.borderColor = '#000000';
		} else {
			id.style.borderColor = '#CCCCCC';
		}
	}
}

function process_key(key, alt)
{
<?php
	if ($_SESSION['user'] != 'demo') {
?>
	if (key == 's' && alt) {
		do_save();
		return;
	}
	if (key == 'n' && alt) {
		do_next();
		return;
	}
<?php
	}
?>
	change_button(key);
}

function buttonClick(e)
{
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();

	var num = /^[0-9]+$/;
	if (this.id.substr(0, 1) == 'b' && num.test(this.id.substr(1))) {
		change_button(this.id.substr(1));
	}
	return false;
}


function urlKeyPress(e)
{
        var event = e;
        if (!event)
                event = window.event;
        event.cancelBubble = true;
        if (event.stopPropagation)
                event.stopPropagation();
	return true;
}

function keyPress(e)
{
        var event = e;
        if (!event)
                event = window.event;
        event.cancelBubble = true;
        if (event.stopPropagation)
                event.stopPropagation();
        /* switch button shortcuts */
        var code = event.keyCode ? event.keyCode : event.which;
        var c = String.fromCharCode(code);
	process_key(c, event.altKey);
	parent._main.annotation_process_key(c, code, event.altKey);
	return true;
}

function get_base_uri()
{
	var url = parent._top.location.href;
	url = url.substr(0, url.lastIndexOf('/')+1);
	return url;
}

function loadPage(path)
{

	parent._main.location.href = get_base_uri()+'proxy.php/'+path;
	active_page_path = path;
	/* page selection box should reflect new page */
	var e = document.getElementById('page').firstChild;
	while (e != undefined) {
		if (e.nodeName == "OPTION" && e.value == path) {
			e.selected = true;
			break;
		}
		e = e.nextSibling;
	}
	//parent._main.document.getElementsByTagName('body')[0].focus();
}

function urlLoadPage(e)
{
	if (parent._main.show_info_box)
		parent._main.show_info_box('Loading...');
	parent._main.location.href = get_base_uri()+'load.php?url='+document.getElementById('url').value;
	return false;
}

function savePage(hide_box)
{
	var active = parent._main.active;
	var base = parent._main.base;
	var n = 0;
	if (active != undefined && base != undefined) {
		parent._main.busy = 1;
		//parent._main.show_info_box('Saving...');
		n = parent._main.annotation_SetInfo(active);
		/* update page select box */
		var sel = document.getElementById('page');
		if (sel != undefined) {
			var e = sel.firstChild;
			while (e != undefined) {
				if (e.nodeName == "OPTION" && e.value == active_page_path) {
					if (n > 0 && e.text.substr(0, 2) != '* ')
						e.text = '* '+e.text;
					else if (n == 0 && e.text.substr(0, 2) == '* ')
						e.text = e.text.substr(2);
				}
				e = e.nextSibling;
			}
		}
		if (hide_box)
			parent._main.hide_info_box();
		parent._main.busy = 0;
	}
}

function showResult(e)
{
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();

	savePage(1);
	var active = parent._main.active;
	var base = parent._main.base;
	if (active != undefined && base != undefined) {
		window.open(base+'/result.php/'+active, 'Result');
	}
}

function changeGroup(e)
{
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();

	savePage(1);
	var form = document.getElementById('mainform');
	form.submit();
}

function changePage(e)
{
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();

	savePage(0);
	var new_path = getSelected(document.getElementById('page'));
	if (new_path != undefined)
		loadPage(new_path);
}

function do_save()
{
	savePage(1);
}

function saveClicked(e)
{
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();

	do_save();
}

function do_next()
{
	var new_path = getSelectedNext(document.getElementById('page'));
	if (new_path != undefined) {
		savePage(0);
		loadPage(new_path);
	} else {
		savePage(1);
	}
}

function nextClicked(e)
{
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();
	do_next();
}

function onLoad()
{
<?php	global $buttons;
	for ($i = 1; $i <= count($buttons); $i++) { ?>
	document.getElementById('b<?php print $i; ?>').onclick = buttonClick;
<?php	} ?>

	document.getElementById('urlbar').onkeypress = urlKeyPress;
	/* demo */
	if (document.getElementById('urlload') != undefined) {
		document.getElementById('urlload').onclick = urlLoadPage;
		document.getElementById('urlload').onkeypress = keyPress;
		document.getElementById('result').onclick = showResult;
		document.getElementById('result').onkeypress = keyPress;
	}
	/* not demo */
	var group = document.getElementById('group');
	var page = document.getElementById('page');
	if (group != undefined) {
		group.onchange = changeGroup;
		group.onkeypress = keyPress;
		if (page != undefined) {
			page.onchange = changePage;
			page.onkeypress = keyPress;
			var first = getSelected(document.getElementById('page'));
			loadPage(first);
		} else {
			parent._main.location.href = get_base_uri()+'empty.html';
		}
		if (document.getElementById('save') != undefined) {
			document.getElementById('save').onclick = saveClicked;
			document.getElementById('save').onkeypress = keyPress;
		}
		if (document.getElementById('next') != undefined) {
			document.getElementById('next').onclick = nextClicked;
			document.getElementById('next').onkeypress = keyPress;
		}
	}
	var b = document.getElementById('b2');
	if (b != undefined)
		b.focus();
}

</script>
</head>
<body id="mainbody" onload="onLoad();" onkeypress="keyPress(event);">
<form id="mainform" action="#">
<div id="buttonbar">
<?php	global $buttons;
	for ($i = 1; $i <= count($buttons); $i++) { ?>
<input id="b<?php print $i; ?>" type="button" value="<?php print $buttons[$i-1]; ?>" />
<?php	} ?>
</div>
<div id="urlbar">
<?php
	if ($_SESSION['user'] == 'demo') {
		print "URL:\n";
		print "<input id=\"url\" type=\"text\" value=\"http://\" />\n";
		print "<input id=\"urlload\" type=\"submit\" value=\"Load\" />\n";
	} else {
		$data = $_SESSION['data_dir'];
		print "Group:\n";
		print "<select id=\"group\" name=\"group\">\n";
		/* find available groups */
		$group = isset($_GET['group']) && is_numeric($_GET['group']) ? $_GET['group'] : 0;
		$f = get_user($_SESSION['user']);
		array_shift($f);
		array_shift($f);
		$i = 0;
		foreach ($f as $x) {
			print "<option value=\"$i\"";
			if ($group == $i) {
				$group_name = $x;
				print " selected=\"selected\"";
			}
			print ">$x</option>\n";
			$i++;
		}
		print "</select>\nPage:\n";
		/* open group file, read entries */
		$count = 0;
		if (isset($group_name)) {
			$fh = @fopen($_SESSION['group_dir'].'/'.$group_name.'.group', "r");
			if ($fh) {
				print "<select id=\"page\" name=\"page\">\n";
				while (!feof($fh)) {
					$l = fgets($fh, 4096);
					$l = chop($l);
					$f = split("[ \t]+", $l);
					if (count($f) != 3)
						continue;
					if ($f[1] != 'OK')
						continue;
					$item = ereg_replace("^http://([^/]{1,15})([^/]*)?(/.{1,10})?.*", "\\1...\\3", $f[0]);
					$item = ereg_replace("\.\.\.$", "", $item);
					$fn = $data.'/'.$f[2].'.xml';
					if (file_exists($fn)) {
						system("grep -E \"<ao[0-9]* value=.[^0]|<at[0-9]* \" '$fn' >/dev/null 2>&1", $retval);
						if (!$retval)
							$item = "* $item";
					}
					print "<option value=\"$f[2]\"";
					if ($i == 0)
						print "selected=\"selected\"";
					print ">$item</option>\n";
					$count++;
				}
				fclose($fh);
				print "</select>\n";
			}
		}
		if ($count == 0) {
			print "Not found ";
		} else {
			print "<input id=\"next\" type=\"button\" value=\"Next\" />\n";
			print "<input id=\"save\" type=\"button\" value=\"Save\" />\n";
		}
	}
?>
</div>
<?php
	if ($_SESSION['user'] == 'demo') {
		print "<div id=\"resultbar\">\n";
		print "<input id=\"result\" type=\"button\" value=\"View\" />\n";
		print "</div>\n";
		print "<div id=\"helpbar\">\n";
		print "<a href=\"help.php\" target=\"_help\">Help</a> | <a href=\"login.php\" target=\"_top\">Login</a>\n";
		print "</div>\n";
	} else {
		print "<div id=\"helpbar\">\n";
		print "<a href=\"help.php\" target=\"_help\">Help</a> | <a href=\"login.php\" target=\"_top\">Logout</a>\n";
		print "</div>\n";
	}
?>
</form>
<div id="clear"></div>
</body>
</html>
