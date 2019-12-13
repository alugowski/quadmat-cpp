"""
Generator for some QuadMat unit test matrices.
"""

from scipy.sparse import csc_matrix
import scipy.io
import os.path

from . import unit_test_matrix_dir, unit_test_multiply_matrix_dir


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
