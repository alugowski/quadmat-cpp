#!/usr/bin/env python3

import scipy.io
import os.path

test_matrix_dir = os.path.join("..", "test", "matrices")
"""
Directory where to place generated matrices. Tests will read from here.
"""


def generate_kg_square():
    """
    Generate a matrix that is the square of the Kepner-Gilbert graph
    """
    m = scipy.io.mmread(os.path.join(test_matrix_dir, "kepner_gilbert_graph.mtx"))
    res = m * m
    scipy.io.mmwrite(os.path.join(test_matrix_dir, "kepner_gilbert_graph_squared.mtx"), res, field='integer')


def main():
    generate_kg_square()


if __name__ == "__main__":
    main()
