#!/bin/bash
diff -U 0 $1 $2 > ${2%.*}.fp_diff || true
cat ${2%.*}.fp_diff | sed -e '/^--- /d;/^+++ /d;/^@@ /d' > ${2%.*}.diff
