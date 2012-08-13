#!/bin/sh
# Clean up after a failed build.
#
# Requires access to .gitignore files excluding _all_ modified files.
#
# Requires a working /dev/fd (with more than just /dev/fd/0 and 1)
# or gawk.

set -e

splitgitignore='#!/usr/bin/awk
!/^#/ && !/^$/ {
	glob = /[[*?]/;
	directory = /\/$/;
	sub(/\/$/, "");
	anchored = /\//;
	sub(/^\//, "");

	output = "nonexistent/nonsense";
	if (anchored) {
		if (!directory && !glob)
			output = "/dev/fd/1";
		else if (directory && !glob)
			output = "/dev/fd/3";
		else if (!directory && glob)
			output = "/dev/fd/4";
		else if (directory && glob)
			output = "/dev/fd/5";
	} else {
		if (!directory)
			output = "/dev/fd/6";
		else
			output = "/dev/fd/7";
	}
	print >> output;
}
'

offlimits="-type d -name '.*' -prune -o -type d -name debian -prune"

remove_file_globs() {
	while read glob
	do
		eval "rm -f $glob"
	done
}

remove_directory_globs() {
	while read glob
	do
		eval "rm -fr $glob"
	done
}

remove_file_findpatterns() {
	while read pat
	do
		find . $offlimits -o \
			'(' -name "$pat" -execdir rm -f '{}' + ')'
	done
}

remove_directory_findpatterns() {
	while read pat
	do
		find . $offlimits -o \
			'(' -type d -name "$pat" -execdir rm -fr '{}' + ')'
	done
}

find . $offlimits -o '(' -name .gitignore -print ')' |
while read file
do
	(
		cd "$(dirname "$file")"
		# Dispatch using pipes.  Yuck.
		{ { { { {
			awk "$splitgitignore" |
		{
			# anchored files (globless)
			xargs -d '\n' rm -f
		}
		} 3>&1 >&2 |
		{
			# anchored directories (globless)
			xargs -d '\n' rm -fr
		}
		} 4>&1 >&2 |
		{
			# anchored files
			remove_file_globs
		}
		} 5>&1 >&2 |
		{
			# anchored directories
			remove_directory_globs
		}
		} 6>&1 >&2 |
		{
			# unanchored files
			remove_file_findpatterns
		}
		} 7>&1 >&2 |
		{
			remove_directory_findpatterns
		} >&2
	) < "$file"
done
