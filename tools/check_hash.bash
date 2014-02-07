#!/bin/bash

# Default to your release branch on the local machine, but allow for
# checking another branch.
branch="release"
if [[ "$1" != "" ]]
then
    branch="$1"
fi

git log $branch >& /dev/null
if [[ "$?" != 0 ]]
then
    exit 1
fi

name="$(git log $branch | grep ^Author | head -n1)"
date="$(git log $branch | grep ^Date | head -n1)"
key="$(echo "$name$date" | sha1sum | cut -d' ' -f1)"

hash="$(git log $branch | grep ^commit | head -n1 | cut -d' ' -f2)"

echo $key $hash
