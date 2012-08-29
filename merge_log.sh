#!/bin/bash
grep '^+net\|^+r0.*v64_'|awk '/^+r0.*v64_/{printf "%s %s\n",$0,x};{x=$0};'|sort

