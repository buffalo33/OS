#!/usr/bin/python3

import sys, os, subprocess
import argparse
import numpy as np
import matplotlib.pyplot as plt
import time
import multiprocessing as mp

__authors__ = ['Clément Preaut', 'Baptiste Lecat', 'Tom Moënne-Loccoz', 'Aurélien Moinel', 'Fabien Savy']
__credits__ = ['Clément Preaut', 'Baptiste Lecat', 'Tom Moënne-Loccoz', 'Aurélien Moinel', 'Fabien Savy']
__version__ = '1.2.0'
__maintainer__ = 'it202-12948 team'
__status__ = 'Prototype'
__course__ = 'it202'
__date__ = '16/04/2021'
__description__ = 'Graph Generation'

def header_print():
    print('# ' + '=' * 88)
    print('Authors: ' +', '.join(__authors__))
    print('Credits: ' + ', '.join(__credits__))
    print('Version: ' + __version__)
    print('Maintainer: ' + __maintainer__)
    print('Status: ' + __status__)
    print('Course: ' + __course__)
    print('Date: ' + __date__)
    print('Description: ' + __description__)
    print('# ' + '=' * 88)

def main():

    header_print()

    args = parse_args()

    file_test_opts = {
        **dict.fromkeys(['21', '22', '23', '51', '52', '53', '54', '55', '56'], [f"{args.n}", f"{args.s}", f"{args.i}", ""]),
        **dict.fromkeys(['31', '32', '33'], [f"{args.n}", f"{args.s}", f"{args.i}", f"{args.y}"])
    }

    # We retrive files and associated options
    files, opts = get_tests_opts(args.file, file_test_opts)

    # Array used for the line graphs (abscissa value)
    n_array = np.array([i*args.s for i in range(1, int(args.n/args.s)+1)])

    # Then we generate the graph
    func_gen_graph, res_arrays = choose_comparison_graph(opts, files)

    func_gen_graph(res_arrays, n_array)



def parse_args():
    """
        Parse the given scripts arguments.

        :return: The parsed arguments
        :rtype: namespace
    """
    parser = argparse.ArgumentParser(description='Executes python tests.')

    parser.add_argument('file', metavar='test_number', type=str,
                        help='The associated file test number')
    parser.add_argument('-n', type=int, default=20,
                        help='Indicates the number of threads to use (default: use 100 threads)')
    parser.add_argument('-s', type=int, default=1,
                        help='Indicates the step used to generate graph (default: use a step of 1)')
    parser.add_argument('-y', type=int, default=20,
                        help='Indicates the number of yields to use (default: use 100 yields)')
    parser.add_argument('-i', type=int, default=20,
                        help='Indicates the number of iterations to use for one test (default: use 20 iterations)')
    args = parser.parse_args()
    return args


def get_tests_opts(test_nb, file_test_opts):
    """
        Get the associated test files and options from
        the given arguments of the script.

        :param test_nb: The number of the desired test file.
        :param file_test_opts: The dictionnary of possible options.
        :type test_nb: str
        :type file_test_opts: dict
        :return: A tuple of the files to test and associated options.
        :rtype: tuple
    """
    cmd = f"ls ./install/bin/{test_nb}*"
    ls = os.popen(cmd).read().split("\n")[:-1]
    opts = file_test_opts.get(f"{test_nb}", [])
    return ls, opts


def choose_comparison_graph(opts, files):
    """
        Generate the right graph according to the number of arguments.

        :param opts: The number of the desired test file.
        :param files: The dictionnary of possible options.
        :type opts: array[str]
        :type file_test_opts:
        :return: An array tuple resulting from the performance calculation
        :rtype: tuple(ndarray)
    """
    queue = mp.Queue()
    processes = np.array([])

    graph_type = ''
    it = 100

    if (len(opts) == 0):
        t1 = mp.Process(target=simple_plot, args=(it, files[0], '0', queue,))
        t2 = mp.Process(target=simple_plot, args=(it, files[1], '1', queue,))
        graph_type = generate_box_graph
    else:
        it = int(opts[2])
        t1 = mp.Process(target=line_plot, args=(opts, it, files[0], '0', queue,))
        t2 = mp.Process(target=line_plot, args=(opts, it, files[1], '1', queue,))
        graph_type = generate_line_graph

    processes = np.append(processes, t1)
    processes = np.append(processes, t2)
    t1.start()
    t2.start()
    t1.join()
    t2.join()

    # Retrieve process return value
    res = [graph_type]
    res.append([queue.get() for _ in processes])

    return res


def simple_plot(it, file, num_process, queue):
    """
        Create the res array of each execution related to
        test without option.

        :param it: The number of time that experience is realised for one execution.
        :param file: The test file.
        :param num_process: The value of the process on which executions are realised.
        :param queue: The queue in with process is executed.
        :type it: int
        :type file: str
        :type num_process: str
        :type queue: Queue
    """
    res_array = np.array([])
    for _ in range (0, it):
        begin = time.time()
        subprocess.run(["taskset", "-c", num_process, file], stdout=subprocess.DEVNULL)
        end = time.time()

    # Store the difference between the end and the begin of the execution
        res_array = np.append(res_array, end - begin)

    queue.put(res_array)

def line_plot(opts, it, file, num_process, queue):
    """
        Create the res array of each execution related to
        test with one or more options.

        :param opts: The given options.
        :param it: The number of time that experience is realised for one execution.
        :param file: The test file.
        :param num_process: The value of the process on which executions are realised.
        :param queue: The queue in with process is executed.
        :param opts: array[str]
        :type it: int
        :type file: str
        :type num_process: str
        :type queue: Queue
    """

    # Array to store the value of multiple execution for the same number of thread
    mean_array = np.zeros(it)

    res_array = np.array([])

    for i in range (0, int(opts[0]), int(opts[1])):
        for j in range (0, it):
            begin = time.time()
            subprocess.run(["taskset", "-c", num_process, file, str(i), opts[3]], stdout=subprocess.DEVNULL)
            end = time.time()

            mean_array[j] = end - begin

        #Store the median into the result array
        res_array = np.append(res_array, np.median(mean_array))
    queue.put(res_array)


def generate_line_graph(res_arrays, step):
    """
        Generate line graph.

        :param res_arrays: The number of the desired test file.
        :param step: The dictionnary of possible options.
        :type res_arrays: array[array[double]]
        :type step: array[int]
    """
    y = res_arrays[0]
    y2 = res_arrays[1]

    plt.xlabel('n')
    plt.ylabel('Time(s)')
    plt.ticklabel_format(axis="y", style="sci", scilimits=(0,0))

    plt.plot(step, y, label="uthread")
    plt.plot(step, y2, label="pthread")

    plt.legend()

    plt.show()

def generate_box_graph(res_array, step):
    """
        Generate box graph.

        :param res_arrays: The number of the desired test file.
        :param step: The dictionnary of possible options.
        :type res_arrays: array[array[double]]
        :type step: array[int]
    """
    plt.boxplot(res_array, sym='', labels=['uthread', 'pthread'])
    plt.grid(True, axis='y')
    plt.ylabel('Time(s)')
    plt.ticklabel_format(axis="y", style="sci", scilimits=(0,0))
    plt.legend()
    plt.show()


if __name__ == '__main__':
   main()
