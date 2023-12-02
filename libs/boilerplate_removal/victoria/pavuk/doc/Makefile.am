## Process this file with automake to produce Makefile.in

#
# Note: the generated HTML formatted man page is included in the distro as the tools required to generate
# the HTML version of the man page cannot be assumed to be available on the target system.
# And documentation is too important to make its availability dependent on several developer
# tools that the user may or may not have installed.
#
EXTRA_DIST = \
	Tech.Notes.html index.html ceottwk.html News.html Measurements.html \
	chunky.css \
	Pavuk.HOWTO.html Pavuk.Help.html Pavuk.ManPage.html \
	rman-filtering.awk


DISTCLEANFILES = 
CLEANFILES = *~ *.bak


RMAN = rman
RMAN_OPTIONS = -f html
AWK = awk
HTMLTIDY = tidy
HTMLTIDY_OPTIONS = -m -i -wrap 90 -c -asxhtml
DB2X_XSLTPROC = db2x_xsltproc
DB2X_MANXML = db2x_manxml
DB2X_TEXIXML = db2x_texixml



pavuk.mxml: pavuk.docbook.xml
	$(DB2X_XSLTPROC) -C /usr/local/share/docbook2X/xslt/catalog.xml -s man  pavuk.docbook.xml -d -o pavuk.mxml

pavuk.1: pavuk.mxml
	$(DB2X_MANXML) --to-stdout pavuk.mxml > pavuk.1

pavuk.texi: pavuk.mxml
	$(DB2X_TEXIXML) pavuk.mxml


.PHONY: htmltidy man-html

man-html: pavuk.1
	@echo "Making HTML from man page in ./doc/"; 
	($(RMAN) $(RMAN_OPTIONS) ./pavuk.1 | $(AWK) -f rman-filtering.awk > ./Pavuk.ManPage.html );
	@echo "Make sure to run 'make htmltidy' when you're done!"; 

htmltidy:
	@echo "Reformatting HTML pages in ./doc/"; 
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./Tech.Notes.html;
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./index.html;
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./ceottwk.html;
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./News.html;
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./Measurements.html;
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./Pavuk.ManPage.html;
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./Pavuk.HOWTO.html;
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./Pavuk.Help.html;

