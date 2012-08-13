#!/bin/awk -f
# Generate debian/changelog.upstream from debian/changelog and
# the git revision log.  Inspired by Gerrit Pape’s
# debian/changelog.upstream.sh, from the git-core Debian package.
#
# Requires a working /dev/stderr.
#
# Usage:
#	dpkg-parsechangelog --format rfc822 --all |
#	awk -f debian/changelog.upstream.awk

# If argument matches /^Version: /, return remaining text.
# Result is nonempty if and only if argument matches.
function version_line(line) {
	if (line ~ /^Version: /) {
		sub(/^Version: /, "", line);
		return line;
	}
	return "";
}

# If argument matches /^\*.* from commit /, return remaining text.
# Result is nonempty if and only if argument matches.
function commit_id_line(line) {
	if (line ~ / from commit /) {
		sub(/^.* from commit /, "", line);
		sub(/[(][Cc]loses.*/, "", line);
		sub(/[^0-9a-f]*$/, "", line);
		return line;
	}
	return "";
}

# Read standard input, scanning for a changelog entry of the
# form “* New snapshot, taken from commit <blah>.”
# Result is <blah>.
# Result is empty and writes a message to standard error if no such entry is
# found before the next Version: line with a different upstream
# version (or EOF).
# Argument is the upstream version sought.
function read_commit_id(upstream, line,version,corresponding_upstream,commit) {
	while (getline line) {
		version = version_line(line);
		corresponding_upstream = version;
		sub(/-[^-]*$/, "", corresponding_upstream);
		if (version != "" && corresponding_upstream != upstream)
			break;

		commit = commit_id_line(line);
		if (commit != "")
			return commit;
	}

	print "No commit id for " upstream >> "/dev/stderr";
	return "";
}

BEGIN {
	last = "none";
	last_cid = "none";
	cl = "debian/changelog.upstream";
}

# Add a list of all revisions up to last to debian/changelog.upstream
# and set last = new_cid.
# new is a user-readable name for the commit new_cide.
function add_version(new,new_cid, limiter,versionline,command,line) {
	if (last == "none") {
		printf "" > cl;
		last = new;
		last_cid = new_cid;
		return 0;
	}

	if (new == "none") {
		versionline = "Version " last;
		limiter = "";
	} else {
		versionline = "Version " last "; changes since " new ":";
		limiter = new_cid "..";
	}
	print versionline >> cl;
	gsub(/./, "-", versionline);
	print versionline >> cl;

	print "" >> cl;
	command = "git shortlog \"" limiter last_cid "\"";
	while(command | getline line)
		print line >> cl;

	if (new != "none")
		print "" >> cl;
	last = new;
	last_cid = new_cid;
}

{
	version = version_line($0);
	if (version != "") {
		# strip Debian revision
		upstream_version = version;
		sub(/-[^-]*$/, "", upstream_version);

		commit = read_commit_id(upstream_version);
		if (commit == "")
			exit 1;
		add_version(upstream_version, commit);
	}
}

END {
	add_version("none", "none");
}
