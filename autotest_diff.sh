#!/usr/bin/env bash
diff -U 0 $1 $2 > ${2%.*}.fp_diff

./fp2bit $2 ${2%.*}.f2b || exit $?
./bit2fp --no-fp-header ${2%.*}.f2b > ${2%.*}.b2f || exit $?
if [ "$1" == "/dev/null" ]
then
	diff -U 0 /dev/null ${2%.*}.b2f > ${2%.*}.b2f_diff
else
	diff -U 0 ${1%.*}.b2f ${2%.*}.b2f > ${2%.*}.b2f_diff
fi

echo "fp:" > ${2%.*}.diff
cat ${2%.*}.fp_diff | sed -e '/^--- /d;/^+++ /d;/^@@ /d' >> ${2%.*}.diff
echo "bit:" >> ${2%.*}.diff
cat ${2%.*}.b2f_diff | sed -e '/^--- /d;/^+++ /d;/^@@ /d' >> ${2%.*}.diff
