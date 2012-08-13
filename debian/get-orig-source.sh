#!/bin/sh
# Build a tarball from the latest upstream version, with a nice
# version number.
#
# Requires git 1.6.6 or later, GNU date, and gzip.

set -e

: ${REPO=$(git rev-parse --git-dir)}
: ${BRANCH=remotes/origin/master}

mkdir debian-orig-source
trap 'rm -fr debian-orig-source || exit 1' EXIT

git init -q debian-orig-source
GIT_DIR=$(pwd)/debian-orig-source/.git
export GIT_DIR

# Fetch latest upstream version.
git fetch -q "$REPO" "$BRANCH"

# Determine version number.
release=0.0
date=$(date --utc --date="$(git log -1 --pretty=format:%cD FETCH_HEAD)" "+%Y%m")
upstream_version="${release}+${date}"

# Generate tarball.
echo "packaging $(git rev-parse --short FETCH_HEAD)"
git archive FETCH_HEAD |
	gzip -n -9 >"fpgatools_$upstream_version.orig.tar.gz"
