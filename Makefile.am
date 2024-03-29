# vim:ft=automake
#
# Gearman server and library
# Copyright (C) 2011-2012 DataDifferential LLC, http://datadifferential.com/
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

ACLOCAL_AMFLAGS= -I m4

bin_PROGRAMS=
sbin_PROGRAMS= 
check_PROGRAMS=
noinst_PROGRAMS=
lib_LTLIBRARIES=
noinst_LTLIBRARIES=
check_LTLIBRARIES=
nobase_include_HEADERS=
noinst_HEADERS=
man1_MANS=
man3_MANS=
man8_MANS=
DISTCLEANFILES=
EXTRA_DIST=
CLEANFILES=
BUILT_SOURCES=
TESTS=
XFAIL_TESTS=
.PHONY=
dist_man_MANS=
man_MANS=

#includes additional rules from aminclude.am
@INC_AMINCLUDE@
DISTCLEANFILES+= aminclude.am

# Should always be first
include libtest/include.am
include libhostile/include.am
include libhashkit-1.0/include.am

include libhashkit/include.am
include benchmark/include.am
include bin/include.am
include examples/include.am
include gearmand/include.am
include libgearman-server/include.am
include libgearman/include.am
include libgearman-1.0/include.am
include util/include.am
include tests/include.am
include support/include.am
include docs/include.am
include man/include.am

include m4/include.am

TESTS += $(bin_PROGRAMS)
TESTS += $(check_PROGRAMS)
XFAIL_TESTS += $(bin_PROGRAMS)

EXTRA_DIST+= HACKING
EXTRA_DIST+= scripts/README.solaris

test: check

BUILT_SOURCES+= configmake.h
CLEANFILES+= configmake.h configmake.h-t
configmake.h: Makefile
	@rm -f $@-t $@
	@{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  echo '#define SYSCONFDIR "$(sysconfdir)"'; \
	  echo '#define GEARMAND_CONFIG "$(sysconfdir)/gearmand.conf"'; \
	  echo '#define LOCALSTATEDIR "$(localstatedir)"'; \
	  echo '#define GEARMAND_PID "$(localstatedir)/gearmand.pid"'; \
	  } | sed '/""/d' > $@-t
	@if diff $@-t $@ >/dev/null 2>&1 ; then \
	  rm @-t ; \
	  else \
	  mv $@-t $@ ; \
	  fi

clean-local: clean-libtest-check clean-docs-check

maintainer-clean-local:
	find . -type f -name '*~' -exec rm -f '{}' \;
	-rm -f @PACKAGE@-*.rpm
	-rm -f @PACKAGE@-*.tar.gz
	-rm -f man/*.1
	-rm -f man/*.3
	-rm -f man/*.8
	-rm Makefile.in
	-rm aclocal.m4
	-rm build-aux/config.guess
	-rm build-aux/config.sub
	-rm build-aux/depcomp
	-rm build-aux/install-sh
	-rm build-aux/ltmain.sh
	-rm build-aux/missing
	-rm build-aux/test-driver 
	-rm gear_config.in
	-rm config.log
	-rm config.status
	-rm configure
	-rm m4/libtool.m4
	-rm m4/ltoptions.m4
	-rm m4/ltsugar.m4
	-rm m4/ltversion.m4
	-rm m4/lt~obsolete.m4
	-rmdir build-aux
	-rm -rf ${top_builddir}/html

include rpm.am

merge-clean:
	@find ./ | $(GREP) \.gcda | xargs rm -f
	@find ./ | $(GREP) \.gcno | xargs rm -f
	@find ./ | $(GREP) \.gz | xargs rm -f
	@find ./ | $(GREP) \.moved | xargs rm -r -f
	@find ./ | $(GREP) \\.orig | xargs rm -f
	@find ./ | $(GREP) \.rej | xargs rm -f
	@rm -f *\.rpm
	@find ./ | $(GREP) \.THIS | xargs rm -f
	@find ./ | $(GREP) \.OTHER | xargs rm -f
	@find ./ | $(GREP) \.BASE | xargs rm -f
	@find ./ | $(GREP) \~$$ | xargs rm -f
	@echo "Files that need to be either removed or checked in:"
	@bzr unknowns
