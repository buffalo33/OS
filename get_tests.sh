#!/usr/bin/env bash

TEST_DIR="test"
URL="https://goglin.gitlabpages.inria.fr/enseirb-it202/tests/"

mkdir -p $TEST_DIR

# Indicate if the test sources were modified
tests_updated() {
    git diff --quiet $TEST_DIR
    [[ $? -eq 1 ]]
    return
}

# Download the tests from $URL
get_tests() {
    test_names=$(curl $URL 2>/dev/null | egrep --only-matching '[0-9]+-[^">]*\.c?' | uniq)
    for test_name in $test_names
    do
        echo $test_name
        curl $URL$test_name > $TEST_DIR/$test_name 2>/dev/null
    done
}

# Commit the content of $TEST_DIR
commit_updates() {
    git add $TEST_DIR/[0-9]*.c
    git commit -m "test(uthread): Update tests" --author="Brice Goglin <goglin@enseirb-matmeca.fr>"
}


if tests_updated;
then
    echo "It looks like the tests were modified, exiting."
    exit 1
fi

get_tests

if tests_updated;
then
    commit_updates
    echo "-> The tests were updated, a commit was produced."
else
    echo "-> Already up to date."
fi
