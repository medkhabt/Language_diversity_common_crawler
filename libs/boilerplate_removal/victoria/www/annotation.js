/* config variables (defined in another file)
var overlap; 
var buttons;

color definition 
   0x01 - selection, 0x02 - first, 0x04 - second, 0x08 third, ...
   null -- ignored

var bg = new Array('#FFFFFF', '#FFAAAA', null     , null     , '#FFFF00');
var fg = new Array('#000000', '#000000', '#FF0000', '#0000FF', null);

var bg = new Array('#FFFFFF', '#FFAAAA', '#FFE680', '#FFFF80', '#99FF80');
var fg = new Array('#000000', '#000000', '#555555', '#555555', '#555555');
*/

/* misc status */
var annotation_state = new Array();
var annotation_fg = new Array();
var annotation_bg = new Array();
var annotation_reportScript = 'store.php';
var busy = 1;
var span;	/* usually 'SPAN' */;

/* selections */
var sel_begin = null;
var sel_end = null;
var selections = new Array();	/* 4 items: begin, end, mask, span (type) */
var redo = new Array();		/* 4 items: begin, end, mask, type */
var desel_i = null;

/* document base url */
var base = null;
/* active document path */
var active = null;

function rgb2string(rgb)
{
	var a = Array('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F');
	var result = '#';
	for (var i = 0; i < 3; i++) {
		result = result + a[Math.floor(rgb[i]/16)] + a[rgb[i]%16];
	}
	return result;
}

function get_color(a, value)
{
	var color = Array(0, 0, 0);
	var n = 0;
	var i = 1;
	while (value) {
		if (value & 0x01) {
			if (a[i] != undefined) {
				color[0] += parseInt(a[i].substr(1, 2), 16);
				color[1] += parseInt(a[i].substr(3, 2), 16);
				color[2] += parseInt(a[i].substr(5, 2), 16);
				n++;
			}
		}
		value >>= 1;
		i++;
	}
	if (!n) {
		var color = Array(parseInt(a[0].substr(1, 2), 16), parseInt(a[0].substr(3, 2), 16), parseInt(a[0].substr(5, 2), 16));
		return rgb2string(color);
	}
	color[0] = Math.floor(color[0]/n); color[1] = Math.floor(color[1]/n); color[2] = Math.floor(color[2]/n);
	min = 255;
	for (var i = 0; i < 3; i++) {	
		if (255-color[i] < min)
			min = 255-color[i];
	}
	//color[0] += min; color[1] += min; color[2] += min;
	return rgb2string(color);
}

/* e.g. body, 'SPAN', 'at' */
function next_element(e, type, id)
{
	if (e == undefined)
		return null;

	var f;
	while (1) {
		/* go down the DOM tree */
		f = e; e = e.firstChild;
		while (e != undefined) {
			if (e.nodeName == type && e.id.substr(0, 2) == id)
				return e;
			f = e; e = e.firstChild;
		}

		e = f;
		while (1) {
			/* step aside */
			f = e; e = e.nextSibling;
			if (e != undefined) {
				if (e.nodeName == type && e.id.substr(0, 2) == id)
					return e;
				break;	/* back down the sibling */
			}

			/* go up one level */
			e = f.parentNode;
			if (e.nodeName == 'HTML')
				return null;
		}
	}
}

/* e.g. body, 'SPAN', 'at' */
function prev_element(e, type, id)
{
	if (e == undefined)
		return null;

	var f;
	while (1) {
		/* go down the DOM tree */
		f = e; e = e.lastChild;
		while (e != undefined) {
			if (e.nodeName == type && e.id.substr(0, 2) == id)
				return e;
			f = e; e = e.lastChild;
		}

		e = f;
		while (1) {
			/* step aside */
			f = e; e = e.previousSibling;
			if (e != undefined) {
				if (e.nodeName == type && e.id.substr(0, 2) == id)
					return e;
				break;	/* back down the sibling */
			}

			/* go up one level */
			e = f.parentNode;
			if (e.nodeName == 'HTML')
				return null;
		}
	}
}

function hide_info_box()
{
	var div = document.getElementById('annotation_trans');
	div.style.display = 'none';
	var div = document.getElementById('annotation_box');
	div.style.display = 'none';
}

function show_info_box(text)
{
	var div = document.getElementById('annotation_box');
	if (text != undefined && div.firstChild != undefined)
		div.firstChild.firstChild.nodeValue = text;
	div.style.display = 'block';
	var div = document.getElementById('annotation_trans');
	div.style.display = 'block';
}

/* register all element handlers */
function annotation_register(base_path, path)
{
	base = base_path;
	active = path;
	/* select event for all elements */
	var e = document.getElementById('at0');
	/* set global span */
	if (e != undefined)
		span = e.nodeName;
	while (buttons > 0 && e) {
		var i = parseInt(e.id.slice(2));
		annotation_state[i] = 0x00;
		annotation_fg[i] = e.style.color;
		annotation_bg[i] = e.style.background;
		//e.onclick = annotation_Click;
		//there must be onmousedown, otherwise it fights with body.onmousedown
		e.onmousedown = annotation_Click;
		e.onmouseover = annotation_MouseOver;
		e.onmouseout = annotation_MouseOut;
		e = next_element(e, span, 'at');
	}
	/* this is _very_ slow on Gecko browsers */
	/*annotation_max = document.getElementsByName('annotation_maxn')[0].id.slice(3);
	for (var i = 0; i <= annotation_max; i++) {
		var e = document.getElementById('at'+i);
		if (e != undefined) {
			i = parseInt(e.id.slice(2));
			annotation_state[i] = 0x00;
			e.onclick = annotation_Click;
			e.onmouseover = annotation_MouseOver;
			e.onmouseout = annotation_MouseOut;
		}
	}*/
	/* get info */
	annotation_GetInfo(path);
	var body = document.getElementsByTagName('body')[0];
	//body.onkeypress = annotation_keyPress; # done in HTML
	body.onselectstart = function () { return false; } // ie
	//body.onmousedown = function () { return false; } // mozilla
	if (buttons.length > 0)
		body.onmousedown = annotation_body_Click;

	busy = 0;

	hide_info_box();
	if (typeof annotation_onload == 'function')
		annotation_onload();
}

function annotation_GetInfo(path)
{
	/* prepare request */
	var rq = false;
	if (window.XMLHttpRequest) {
		/* common browsers */
		rq = new XMLHttpRequest();
		if (rq.overrideMimeType)
			rq.overrideMimeType('text/xml');
        } else if (window.ActiveXObject) {
		/* MSIE */
		try { rq = new ActiveXObject("Msxml2.XMLHTTP"); } catch (e) {
			try { rq = new ActiveXObject("Microsoft.XMLHTTP"); } catch (e) {}
		}
	}
	if (!rq) {
		alert('Error: Cannot create an XMLHTTP instance');
		return false;
	}

	rq.open('GET', base+'/restore.php/'+path, false);
	rq.send(null);

	if (rq.status == 200) {
		/* parse response and set values */
		var data = rq.responseXML.getElementsByTagName('data');
		if (data.length > 0) {
			var item = data[0].firstChild;
			while (item != undefined) {
				var name = item.nodeName.substr(0, 2);
				var id = item.nodeName.substr(2);
				switch (name) {
				/* text selections */
				case 'at':
					var begin = parseInt(item.getAttribute('start'));
					var end = parseInt(item.getAttribute('end'));
					var type = parseInt(item.getAttribute('type'));
					var x = new Array(begin, end, 0x01 << id, insert_menu(end, id, type));
					selections.push(x);
					mark(begin, end, 0x01 << id, 1);
					break;
				/* options */
				case 'ao':
					var value = parseInt(item.getAttribute('value'));
					set_option_value(id, value);
					break;
				}
				item = item.nextSibling;
			}
		}
	} else {
		var error = rq.responseXML.getElementsByTagName('error');
		alert(rq.status+': '+error[0].getAttribute('text'));
	}
}

function annotation_SetInfo(path)
{
	show_info_box("Saving...");
	file = path.substr(path.lastIndexOf('/')+1);
	var n = 0;

	/* prepare content */
	var boundary = "xsdfsdf";
	var content = new Array('--'+boundary,
				'Content-Disposition: form-data; name="file"; filename="unknown"',
				'Content-Type: text/xml', '', '<data id="'+file+'">');
	/* user info */
	if (parent._top.user != 'demo')
		content.push('\t<user name="'+parent._top.user+'" />');
	
	/* text selections */
	for (var i = 0; i < selections.length; i++) {
		var begin = selections[i][0];
		var end = selections[i][1];
		var flag = selections[i][2];
		var type = selections[i][3];
		if (type != undefined)
			type = ' type="'+type._av+'"';
		else
			type = '';
		n++;
		j = 0;
		while (flag != 0x01) {
			flag >>= 1;
			j++;
		}
		content.push('\t<at'+j+' start="'+begin+'" end="'+end+'"'+type+' />');
	}
	/* selections */
	var options = get_option_values();
	for (var i = 0; i < options.length; i++) {
		var value = options[i];
		content.push('\t<ao'+i+' value="'+value+'" />');
		if (value != 0)
			n++;
	}

	content.push('</data>');
	content.push('--'+boundary+'--');
	var rbody = content.join('\r\n');

	/* create request */
	var rq = false;
	if (window.XMLHttpRequest) {
		/* common browsers */
		rq = new XMLHttpRequest();
		if (rq.overrideMimeType)
			rq.overrideMimeType('text/xml');
        } else if (window.ActiveXObject) {
		/* MSIE */
		try { rq = new ActiveXObject("Msxml2.XMLHTTP"); } catch (e) {
			try { rq = new ActiveXObject("Microsoft.XMLHTTP"); } catch (e) {}
		}
	}
	if (!rq) {
		alert('Error: Cannot create an XMLHTTP instance');
		return false;
	}

	rq.open('POST', base+'/store.php/'+path , false);
	rq.setRequestHeader('Content-Type', 'multipart/form-data; boundary='+boundary); 
	rq.send(rbody);
	if (rq.status != 200) {
		var error = rq.responseXML.getElementsByTagName('error');
		alert(rq.status+': '+error[0].getAttribute('text'));
		return 0;
	}
	return n;
}

function mark(begin, end, mask, set)
{
	var c = document.getElementById('at'+begin);
	if (c == undefined)
		return;
	var i = begin;
	while (i <= end) {
		annotation_state[i] &= ~0x01;
		if (set) {
			annotation_state[i] |= mask;
		} else {
			annotation_state[i] &= ~mask;
		}
		c.style.color = annotation_state[i] ? get_color(fg, annotation_state[i]) : annotation_fg[i];
		c.style.backgroundColor = annotation_state[i] ? get_color(bg, annotation_state[i]) : annotation_bg[i];
		i++;
		c = next_element(c, span, 'at');
	}
}

function get_current_state()
{
	return parent._top.active_button;
}

function get_current_mask()
{
	return 0x01 << get_current_state();
}

/* TODO: move to another place */
function undo_selection(idx)
{
	/* reset */
	var begin = selections[idx][0];
	var end = selections[idx][1];
	var flag = selections[idx][2];
	var type = selections[idx][3];
	if (type != undefined)
		type = type._av;

	var re = new Array(3);
	re[0] = begin; re[1] = end; re[2] = flag; re[3] = type;
	redo.push(re);
	mark(begin, end, flag, 0);
	selections.splice(idx, 1);
	/* repair markup */
	for (var i = selections.length-1; i >= 0; i--) {
		if (selections[i][2] != flag)
			continue;
		if ((selections[i][0] <= begin && selections[i][1] >= begin) ||
			(selections[i][0] >= begin && selections[i][0] <= end)) {
			mark(selections[i][0], selections[i][1], flag, 1);
		}
	}
	delete_menu(end, type);
}

function redo_selection()
{
	/* mark selection */
	if (!redo.length)
		return;
	var item = redo.pop();
	var x = item[2];
	var state = -1;
	while (x) {
		x = x >> 1;
		state++;
	}
	item[3] = insert_menu(item[1], state, item[3]);
	selections.push(item);
	mark(item[0], item[1], item[2], 1);
}

function menu_activated(id)
{
	var e = document.getElementById('am'+id);
	var i = 1;
	var c = e.firstChild.firstChild;
	while (c) {
		if (c.selected) {
			e._av = i;
			break;
		}
		i++;
		c = c.nextSibling;
	}
}

function insert_menu(id, state, value)
{
	if (buttons2[state-1] == undefined)
		return;
	if (value == undefined)
		value = 1;
	var c = document.getElementById('at'+id);
	var span = document.createElement('span');
	span.id = 'am'+id;
	span._as = state;
	span._av = value;
	var html = '<select style="color: '+get_color(fg, 0x01 << state)+'; background-color: '+get_color(bg, 0x01 << state)+'" onchange="menu_activated(\''+id+'\');">';
	for (var i = 0; i < buttons2[state-1].length; i++) {
		html += '<option value="'+i+'"';
		if (i == value-1)
			html += ' selected="selected"';
		html += '>'+buttons2[state-1][i]+'</option>';
	}
	html += '</select>';
	span.innerHTML = html;
	if (c.nextSibling != undefined)
		c.parentNode.insertBefore(span, c.nextSibling);
	else
		c.parentNode.appendChild(span);
	//span.onmousedown = menu_activated;
	return span;
}

function delete_menu(id, type)
{
	if (type == undefined)
		return;
	var c = document.getElementById('am'+id);
	if (c == undefined)
		return;
	c.parentNode.removeChild(c);
}

function set_option_value(id, value)
{
	var select = document.getElementById('ao'+id);
	if (select == undefined)
		return;
	var item = select.firstChild;
	while (item != undefined) {
		if (item.nodeName == 'OPTION') {
			if (item.value == value) {
				item.selected = true;
				break;
			}
		}
		item = item.nextSibling;
	}
}

function get_option_values(id)
{
	var result = new Array();
	var select = document.getElementById('ao0');
	if (select == undefined)
		return result;
	while (select != undefined) {
		var item = select.firstChild;
		var first_value = null;
		while (item != undefined) {
			if (item.nodeName == 'OPTION') {
				if (item.selected) {
					result.push(item.value);
					break;
				}
				if (item.disabled == false && first_value == undefined)
					first_value = item.value;
			}
			item = item.nextSibling;
		}
		if (item == undefined)
			result.push(first_value);
		select = next_element(select, 'SELECT', 'ao');
	}
	return result;
}

function annotation_Click(e)
{
	if (busy)
		return;
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();
	n = parseInt(this.id.slice(2));
	if (annotation_state[n] == undefined)
		return;
	if (sel_begin == undefined) {
		if (event.ctrlKey) {
			if (desel_i != undefined) {
				/* reset */
				undo_selection(desel_i);
				desel_i = null;
			}
		} else {
			/* selection begin */
			if (overlap || (annotation_state[n] == 0x00)) {
				sel_begin = sel_end = n;
				annotation_state[n] |= 0x01;
				this.style.color = annotation_state[n] ? get_color(fg, annotation_state[n]) : annotation_fg[n];
				this.style.backgroundColor = annotation_state[n] ? get_color(bg, annotation_state[n]) : annotation_bg[n];
			}
		}
	} else {
		/* selection end */
		if (sel_begin > sel_end) {
			x = sel_end;
			sel_end = sel_begin;
			sel_begin = x;
		}
		var mask = get_current_mask();
		mark(sel_begin, sel_end, mask, 1);
		var x = new Array(sel_begin, sel_end, mask, insert_menu(sel_end, get_current_state(), 1));
		selections.push(x);
		redo = new Array();
		sel_begin = sel_end = null;
	}
}

function annotation_body_Click(e)
{
	if (busy)
		return false;
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();
	if (sel_begin != undefined) {
		/* selection end */
		if (sel_begin > sel_end) {
			x = sel_end;
			sel_end = sel_begin;
			sel_begin = x;
		}
		var mask = get_current_mask();
		mark(sel_begin, sel_end, mask, 1);
		var x = new Array(sel_begin, sel_end, mask, insert_menu(sel_end, get_current_state(), 1));
		selections.push(x);
		redo = new Array();
		sel_begin = sel_end = null;
	}
	return false;
}

function annotation_update_sel(cur)
{
// original code: allowed overlap, but much more simple and elegant (reset &
// set done in one cycle)
//	var begin, end;
//	if (sel_begin <= sel_end) {
//		if (cur > sel_end) {
//			/* right */
//			begin = sel_end+1;
//			end = cur;
//		} else if (cur < sel_begin) {
//			/* left */
//			begin = cur;
//			end = sel_end;
//		} else {
//			/* between */
//			begin = cur+1;
//			end = sel_end;
//		}
//	} else {
//		if (cur < sel_end) {
//			/* left */
//			begin = cur;
//			end = sel_end-1;
//		} else if (cur > sel_begin) {
//			/* right */
//			begin = sel_end;
//			end = cur;
//		} else {
//			/* between */
//			begin = sel_end;
//			end = cur-1;
//		}
//	}
//	sel_end = cur;
//	var i = begin;
//	var c = document.getElementById('at'+begin);
//	while (i <= end) {
//		if (c.nodeName == 'SPAN') {
//			if (i != sel_begin) {
//				if (!(annotation_state[i] & 0x01))
//					annotation_state[i] |= 0x01;
//				else
//					annotation_state[i] &= ~0x01;
//				c.style.color = fg[annotation_state[i]];
//				c.style.backgroundColor = bg[annotation_state[i]];
//			}
//			i++;
//		}
//		c = c.nextSibling;
//	}

	var set_begin = null, set_end = null;
	var reset_begin = null, reset_end = null;

	if (sel_begin <= sel_end) {
		if (cur > sel_end) {
			/* right */
			set_begin = sel_end+1;
			set_end = cur;
		} else if (cur < sel_begin) {
			/* left */
			reset_begin = sel_begin+1;
			reset_end = sel_end;
			set_begin = sel_begin-1;
			set_end = cur;
		} else {
			/* between */
			reset_begin = cur+1;
			reset_end = sel_end;
		}
	} else {
		if (cur < sel_end) {
			/* left */
			set_begin = sel_end-1;
			set_end = cur;
		} else if (cur > sel_begin) {
			/* right */
			reset_begin = sel_end;
			reset_end = sel_begin-1;
			set_begin = sel_begin+1;
			set_end = cur;
		} else {
			/* between */
			reset_begin = sel_end;
			reset_end = cur-1;
		}
	}
	/* reset unused selection, reset_begin <= reset_end */
	if (reset_begin != undefined) {
		var i = reset_begin;
		var c = document.getElementById('at'+reset_begin);
		while (i <= reset_end) {
			annotation_state[i] &= ~0x01;
			c.style.color = annotation_state[i] ? get_color(fg, annotation_state[i]) : annotation_fg[i];
			c.style.backgroundColor = annotation_state[i] ? get_color(bg, annotation_state[i]) : annotation_bg[i];
			i++;
			c = next_element(c, span, 'at');
		}
	}

	/* set new selection */
	sel_end = cur;
	if (set_begin != undefined) {
		if (sel_begin < set_begin) {
			var i = set_begin;
			var c = document.getElementById('at'+set_begin);
			while (i <= set_end) {
				if (!overlap && ((annotation_state[i] & 0xFE) != 0x00)) {
					sel_end = i-1;
					break; /* not allowed to overlap */
				}
				annotation_state[i] |= 0x01;
				c.style.color = annotation_state[i] ? get_color(fg, annotation_state[i]) : annotation_fg[i];
				c.style.backgroundColor = annotation_state[i] ? get_color(bg, annotation_state[i]) : annotation_bg[i];
				i++;
				c = next_element(c, span, 'at');
			}
		} else {
			var i = set_begin;
			var c = document.getElementById('at'+set_begin);
			while (i >= set_end) {
				if (!overlap && ((annotation_state[i] & 0xFE) != 0x00)) {
					sel_end = i+1;
					break; /* not allowed to overlap */
				}
				annotation_state[i] |= 0x01;
				c.style.color = annotation_state[i] ? get_color(fg, annotation_state[i]) : annotation_fg[i];
				c.style.backgroundColor = annotation_state[i] ? get_color(bg, annotation_state[i]) : annotation_bg[i];
				i--;
				c = prev_element(c, span, 'at');
			}
		}
	}
}

function annotation_reset_desel()
{
	if (desel_i == undefined)
		return;
	mark(selections[desel_i][0], selections[desel_i][1], 0x01, 0);
	desel_i = null;
}

function annotation_update_desel(cur)
{
	var mask = get_current_mask();
	for (var i = selections.length-1; i >= 0; i--) {
		if (selections[i][0] <= cur && cur <= selections[i][1] && mask == selections[i][2])
			break;
	}
	if (i < 0 || desel_i != i)
		annotation_reset_desel();
	if (i >= 0) {
		desel_i = i;
		mark(selections[desel_i][0], selections[desel_i][1], 0x01, 1);
	}
}

function annotation_MouseOver(e)
{
	if (busy)
		return;
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();

	var cur = parseInt(this.id.slice(2));
	if (sel_begin == undefined) {
		if (event.ctrlKey)
			annotation_update_desel(cur);
		else
			annotation_reset_desel();
		return;
	}
	annotation_update_sel(cur);
}

function annotation_MouseOut(e)
{
	if (busy)
		return;
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();

	var cur = parseInt(this.id.slice(2));
	if (sel_begin == undefined)
		annotation_reset_desel();
}

function annotation_process_key(c, code, alt)
{
	/* undo last selection */
	if (c == 'u' && selections.length > 0)
		undo_selection(selections.length-1);
	if (c == 'r' && redo.length > 0)
		redo_selection();
	/* reset current selection */
	if (sel_begin == undefined || code != 27)
		return false;
	if (sel_begin > sel_end) {
		x = sel_end;
		sel_end = sel_begin;
		sel_begin = x;
	}
	mark(sel_begin, sel_end, 0x01, 0);
	sel_begin = sel_end = null;
}

function annotation_keyPress(e)
{
	if (busy)
		return false;
	var event = e;
	if (!event)
		event = window.event;
	event.cancelBubble = true;
	if (event.stopPropagation)
		event.stopPropagation();
	/* switch button shortcuts */
	var code = event.keyCode ? event.keyCode : event.which;
	var c = String.fromCharCode(code);
	parent._top.process_key(c, event.altKey);
	annotation_process_key(c, code, event.altKey);
	return false;
}
