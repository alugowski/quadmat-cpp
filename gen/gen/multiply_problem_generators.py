"""
Generator for multiply problems. These include matrices A, B, and sometimes C, as well as the expected product matrix.
"""

import numpy as np
import scipy.io
from scipy.sparse import csc_matrix
import os.path
from . import matrix_generators as matgen
from . import unit_test_matrix_dir


def generate_multiply_problem(basedir, name, a, b, c=None,
                              a_field='integer', b_field='integer', c_field='integer',
                              a_symmetry='general', b_symmetry='general', c_symmetry='general'):
    """
    Given input matrices, compute results and save everything in a directory where medium tests will pick it up.
    """

    dirname = os.path.join(basedir, name)
    os.makedirs(dirname, exist_ok=True)

    scipy.io.mmwrite(os.path.join(dirname, "a.mtx"), a, field=a_field, symmetry=a_symmetry)
    scipy.io.mmwrite(os.path.join(dirname, "b.mtx"), b, field=b_field, symmetry=b_symmetry)

    if c is None:
        ab = a.dot(b)

        product_ab_field = a_field if a_field == b_field else None
        scipy.io.mmwrite(os.path.join(dirname, "product_ab.mtx"), ab, field=product_ab_field, symmetry='general')
    else:
        abc = a * b * c
        scipy.io.mmwrite(os.path.join(dirname, "c.mtx"), c, field=c_field, symmetry=c_symmetry)

        product_abc_field = a_field if a_field == b_field == c_field else None
        scipy.io.mmwrite(os.path.join(dirname, "product_abc.mtx"), abc, field=product_abc_field, symmetry='general')


def generate_kg_square_problem(basedir, problem_name="square Kepner-Gilbert graph"):
    """
    Generate a matrix that is the square of the Kepner-Gilbert graph
    """
    m = scipy.io.mmread(os.path.join(unit_test_matrix_dir, "kepner_gilbert_graph.mtx"))
    m = csc_matrix(m)

    generate_multiply_problem(
        basedir=basedir,
        name=problem_name,
        a=m,
        b=m,
        a_field='integer',
        b_field='integer',
    )


def generate_er_problems(basedir, scale, fill_factor=32):
    """
    Generate a problem with two Erdos-Renyi matrices are multiplied together.
    """
    np.random.seed(scale)

    er = matgen.generate_er_matrix(2**scale, 2**scale, nnn=2**scale*fill_factor, dedupe=True)
    perm = matgen.generate_permutation_matrix(er.get_shape()[0])
    left_sub, right_sub = matgen.generate_submatrix_extraction(er.get_shape())

    generate_multiply_problem(
        basedir=basedir,
        name=f"square ER scale {scale}",
        a=er,
        b=er)

    generate_multiply_problem(
        basedir=basedir,
        name=f"row_perm ER scale {scale}",
        a=perm,
        b=er)

    generate_multiply_problem(
        basedir=basedir,
        name=f"submatrix ER scale {scale}",
        a=left_sub,
        b=er,
        c=right_sub)


def generate_torus_problems(basedir, scale):
    """
    Generate problems involving a 3D torus.
    """
    np.random.seed(scale)

    torus = matgen.generate_3d_torus_matrix(scale)
    perm = matgen.generate_permutation_matrix(torus.get_shape()[0])
    torus_rp = perm * torus * perm.transpose()

    generate_multiply_problem(
        basedir=basedir,
        name=f"square 3Dtorus scale {scale}",
        a=torus,
        b=torus)

    generate_multiply_problem(
        basedir=basedir,
        name=f"square 3DtorusRP scale {scale}",
        a=torus_rp,
        b=torus_rp)

    generate_multiply_problem(
        basedir=basedir,
        name=f"row_perm 3Dtorus scale {scale}",
        a=perm,
        b=torus)

    generate_multiply_problem(
        basedir=basedir,
        name=f"row_perm 3DtorusRP scale {scale}",
        a=perm,
        b=torus_rp)
