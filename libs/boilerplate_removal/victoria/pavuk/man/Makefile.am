
#
# Note: the generated HTML formatted man page is included in the distro as the tools required to generate
# the HTML version of the man page cannot be assumed to be available on the target system.
# And documentation is too important to make its availability dependent on several developer
# tools that the user may or may not have installed.
#
EXTRA_DIST = \
	Pavuk.ManPage.html \
        pavuk.1.in


man_MANS = pavuk.1

DISTCLEANFILES = pavuk.1
CLEANFILES = *~ *.bak

RMAN = rman
RMAN_OPTIONS = -f html
AWK = awk
HTMLTIDY = tidy
HTMLTIDY_OPTIONS = -m -i -wrap 90 -c -asxhtml

.PHONY: htmltidy man-html

man-html: all-am pavuk.1
	@echo "Making HTML from man page in ./man/"; 
	($(RMAN) $(RMAN_OPTIONS) ./pavuk.1 | $(AWK) -f ../doc/rman-filtering.awk > ./Pavuk.ManPage.html );
	@echo "Make sure to run 'make htmltidy' when you're done!"; 

htmltidy:
	@echo "Reformatting HTML pages in ./man/"; 
	-$(HTMLTIDY) $(HTMLTIDY_OPTIONS) ./Pavuk.ManPage.html;

