#!/usr/bin/bash

mkdir -p tests/results

TIME_PROGRAM="/usr/bin/time"
if [ ! -f $TIME_PROGRAM ]; then
	echo "time command is not installed!"
	exit 1
fi

if [[ $* == *--dry* ]]; then
	GIT_TAG=""
	OUT_FILE="/dev/null"
else
	GIT_TAG=$(git describe --abbrev=10 --dirty --always --tags)
	OUT_FILE=tests/results/$(date -Iseconds)-$GIT_TAG.txt
	echo "Saving results to $OUT_FILE"
fi

if [[ $GIT_TAG == *dirty* ]]; then
	echo "Caution: The git repository contains uncommitted changes!"
	echo "Running and saving the test results is dangerous, " \
		 "because the exact state could not be reproducible later anymore."
	echo
	echo "Run 'make tests-dry' to just run but not save the results."
	exit 1
fi

if compgen -G "tests/results/*$GIT_TAG*" > /dev/null; then
	echo "Caution: A measurement already exists for this git commit hash!"
	echo
	echo "Run 'make tests-dry' to just run but not save the results."
	exit 1
fi

touch $OUT_FILE

for script in tests/*.dm; do
	echo -n $script | tee -a $OUT_FILE
	echo -n ": " | tee -a $OUT_FILE
	# i don't exactly know how this works, but it works
	/usr/bin/time -f '%e' ./bin/diamond $script > /dev/null 2> >(tee -a $OUT_FILE >&2)
done
