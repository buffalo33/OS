import os
import sys
import subprocess
import argparse
import multiprocessing as mp

RED = "\033[31m"
GREEN = "\033[32m"
RESET = "\033[39m"


def print_status(success):
    """
        Pretty-print the success status of an experiment

        :param sucess: the boolean to prettyy-print
    """
    if success:
        print(f"{GREEN}[OK]{RESET}")
    else:
        print(f"{RED}[FAILED]{RESET}")


def parse_options():
    """
        Parse the given scripts arguments.

        :return: The parsed arguments
    """
    parser = argparse.ArgumentParser(description='Launch thread tests.')

    parser.add_argument('--quiet', dest='quiet', action='store_true',
                        help='do not display test output')
    parser.set_defaults(quiet=False)
    parser.add_argument('--valgrind', dest='valgrind',
                        action='store_true', help="run the tests in valgrind")
    parser.set_defaults(valgrind=False)

    parser.add_argument('test_suite', type=str, nargs='*',
                        help='the identifiers of the tests to run')

    args = parser.parse_args()
    return args


if __name__ == '__main__':

    args = parse_options()
    std_redirections = {"stdout": subprocess.DEVNULL,
                        "stderr": subprocess.DEVNULL} if args.quiet else {}

    regex = "[[:digit:]]" if not args.test_suite else f"({'|'.join(args.test_suite)})"
    cmd = f"find ./build | sort | egrep '{regex}'"
    print(cmd)
    ls = os.popen(cmd).read().split("\n")[:-1]
    valgrind = []

    if args.valgrind:
        valgrind = [
            "valgrind",
            "--leak-check=full",
            "--show-reachable=yes",
            "--track-origins=yes",
            "--error-exitcode=1",
            "--exit-on-first-error=yes"
        ]

    global_success = True
    processes = []

    for fl in ls:
        cmd = [f"{fl}", "10", "10"]
        cmd = valgrind + cmd

        p = subprocess.Popen(cmd, **std_redirections)
        processes.append(p)

        if not args.quiet:
            print("\n==== " + ' '.join(cmd) + " ====")
            p.wait()

    for pi in range(len(processes)):
        print(f"{ls[pi]}... ", end="")
        p = processes[pi]
        p.wait()

        success = (p.returncode == 0)
        print_status(success)
        global_success &= success

    print_status(global_success)
    exit(0 if global_success else 1)
