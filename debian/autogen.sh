#!/bin/sh
# Generate debian/changelog.upstream.
#
# Uses debian/changelog and the git revision log.
#

set -e

dpkg-parsechangelog --format rfc822 --all |
	awk -f debian/changelog.upstream.awk
