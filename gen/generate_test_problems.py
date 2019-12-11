#!/usr/bin/env python3
"""
Script to generate problems for QuadMat unit and medium tests.
"""

import numpy as np
from scipy.sparse import csc_matrix
import scipy.io
import os.path

from matrix_generators import generate_er_matrix


test_matrix_dir = os.path.join("..", "test", "matrices")
"""
Directory where to place generated matrices.
"""

unit_test_matrix_dir = os.path.join(test_matrix_dir, "unit")
unit_test_multiply_matrix_dir = os.path.join(unit_test_matrix_dir, "multiply")

medium_test_multiply_matrix_dir = os.path.join(test_matrix_dir, "medium", "multiply")


def generate_kg_square_problem(problem_name="Kepner-Gilbert graph squared"):
    """
    Generate a matrix that is the square of the Kepner-Gilbert graph
    """
    m = scipy.io.mmread(os.path.join(unit_test_matrix_dir, "kepner_gilbert_graph.mtx"))
    m = csc_matrix(m)
    res = m.dot(m)

    comment = " This directed graph appears on the cover of \"Graph Algorithms in the Language of Linear Algebra\" " \
              "edited by Jeremy Kepner and John Gilbert"

    dirname = os.path.join(unit_test_multiply_matrix_dir, problem_name)
    os.makedirs(dirname, exist_ok=True)
    scipy.io.mmwrite(os.path.join(dirname, "a.mtx"), m, field='integer', comment=comment)
    scipy.io.mmwrite(os.path.join(dirname, "b.mtx"), m, field='integer', comment=comment)
    scipy.io.mmwrite(os.path.join(dirname, "product_ab.mtx"), res, field='integer')


def generate_unit_test_problems():
    """
    Generate problems for unit tests.

    These need to be small in order to run in milliseconds with unoptimized code.
    """
    # multiply
    generate_kg_square_problem()


def generate_medium_test_er_x_er(scale, fill_factor=32):
    """
    Generate a problem with two Erdos-Renyi matrices are multiplied together.
    """
    mn = 2**scale
    nnn = 2**scale*fill_factor

    a = generate_er_matrix(mn, mn, nnn=nnn, dedupe=True)
    b = generate_er_matrix(mn, mn, nnn=nnn, dedupe=True)
    ab = a.dot(b)

    comment = " Erdos-Renyi matrix"

    dirname = os.path.join(medium_test_multiply_matrix_dir, f"ER x ER scale {scale}")
    os.makedirs(dirname, exist_ok=True)
    scipy.io.mmwrite(os.path.join(dirname, "a.mtx"), a, field='integer', comment=comment)
    scipy.io.mmwrite(os.path.join(dirname, "b.mtx"), b, field='integer', comment=comment)
    scipy.io.mmwrite(os.path.join(dirname, "product_ab.mtx"), ab, field='integer')


def generate_medium_test_problems():
    """
    Generate problems for medium tests.

    These should be small enough to run in a second or two at most.
    """

    generate_medium_test_er_x_er(scale=5)
    generate_medium_test_er_x_er(scale=10)


def main():
    np.random.seed(0)

    generate_unit_test_problems()

    generate_medium_test_problems()


if __name__ == "__main__":
    main()
