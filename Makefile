
# Copyright (c)1998 by Angry Red Planet.
#
# This code is distributed under a modified form of the
# Artistic License.  A copy of this license should have
# been included with it; if this wasn't the case, the
# entire package can be obtained at
# <URL:http://www.angryredplanet.com/>.
#
# ----------------------------------------------------------------------
#
# ARP/Makefile
#
# Build the world.
#
# ----------------------------------------------------------------------
#
# Known Bugs
# ~~~~~~~~~~
#
# ----------------------------------------------------------------------
#
# To Do
# ~~~~~
#
# ----------------------------------------------------------------------
#
# History
# ~~~~~~~
#
# Dec 6, 1998:
#	First public release.
#
#

	SUBMAKES := $(shell echo */Makefile)

include makefiles/submakes
