#!/bin/bash

TIDY_FORCE=1
RUN_TIDY=1
MARK_TAG=span

this=`dirname $0`;

PAVUK="$this"/../pavuk-bin/bin/pavuk
ICONV=iconv
TIDY=tidy
HTMLMARK="$this"/htmlmark.pl
PERL5LIB=$PERL5LIB:"$this"/../perl

if [ -z "$2" ]; then
	echo "Usage: prepare.sh dir url";
	exit;
fi

dir=$1;
url=$2;

if [ "$TIDY_FORCE" = "1" ]; then
	TIDY="$TIDY --force-output 1";
fi

mkdir "$dir" 2>/dev/null;
dir=`( cd "$dir"; readlink -f . )`
rm -f "$dir/.start";
echo "#!/bin/bash" >"$dir/.script"
echo "if [ \"\$2\" -eq 1 -a ! -f \"$dir/.start\" ]; then echo \"\$1\" >\"$dir/.start\"; fi" >>"$dir/.script";
chmod u+rx "$dir/.script"

# download url
$PAVUK -cdir "$dir" -nouse_http11 -store_info -identity "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1) Gecko/20060601 Firefox/2.0 (Ubuntu-edgy)" -lmax 1 -singlepage -dont_limit_inlines -leave_dir -all_to_local -acharset UTF-8 -timeout 5000 -rtimeout 5000 -wtimeout 5000 -max_time 2 -nthreads 10 -norobots -read_css -post_cmd "$dir/.script" "$url"

f=`cat "$dir/.start" 2>/dev/null`
dirlen=$((${#dir}+1));
file=${f:dirlen};

if [ ! -f "$f" -o ! -s "$f" ]; then
	echo "ERROR: Cannot download URL";
	exit;
fi

# find current encoding
dn=`dirname "$f"`;
bn=`basename "$f"`;
enc=`perl -ni -e 'if ($_ =~ /^Content-Type: text\/html;\s*charset=([^\s]+)\s*$/) { print $1; }' <"$dn/.pavuk_info/$bn"`;
if [ -z "$enc" ]; then
	enc=`perl -ni -e 'if ($_ =~ /<meta[^>]*content=["'\'']?text\/html;\s*charset=([^\s"'\'']+)/i) { print $1; exit; }' <"$f"`;
fi;

# rename all files that contain invalid characters from original character set
if ! echo $enc|grep -i '^utf-\?8' >/dev/null; then
	( cd "$dir";
	find * >/tmp/abctmp.$$;
	while read x; do
		y=`echo $x|iconv -f "$enc" -t "utf-8"`;
		if [ "$x" != "$y" ]; then
			mv "$x" "$y";
		fi
	done </tmp/abctmp.$$; )
	rm -f /tmp/abctmp.$$;
fi

# convert to utf8
if [ -n "$enc" ]; then
	if ! $ICONV -c -f "$enc" -t utf8 <"$f" >"$f".tmp1; then
		echo "ERROR: Cannot convert document!";
		exit;
	fi
else
	cp "$f" "$f".tmp1
fi

# run tidy
if [ "$RUN_TIDY" = "1" ]; then
	$TIDY --wrap 0 -asxhtml -utf8 <"$f".tmp1 2>&1 -o "$f".tmp2 | sed -e 's/</\&lt;/g'|sed -e 's/>/\&gt;/g';
	if [ ! -f "$f".tmp2 ]; then
		echo "ERROR: Cannot tidy-up document!";
		exit;
	fi
else
	cp "$f".tmp1 "$f".tmp2
fi

# prepare for annotation
if ! $HTMLMARK --tag=$MARK_TAG <"$f".tmp2 >"$f".tmp3; then
	echo "ERROR: Cannot pre-process document!";
	exit;
fi

mv "$f" "$f".orig
mv "$f".tmp3 "$f";
mv "$f".tmp2 "$f".in;
rm -f "$f".tmp1;

# finally echo html file, so that user can see it
echo "OK $file";
