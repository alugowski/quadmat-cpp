"""
Generator for some QuadMat medium test problems.
"""

import numpy as np
import scipy.io
import os.path

from . import medium_test_multiply_matrix_dir
from . import matrix_generators as matgen


def generate_medium_test_multiply_problem(name, a, b, c=None, gitignored=True,
                                          a_field='integer', b_field='integer', c_field='integer',
                                          a_symmetry='general', b_symmetry='general', c_symmetry='general'):
    """
    Given input matrices, compute results and save everything in a directory where medium tests will pick it up.
    """

    if gitignored:
        name = "gitignored - " + name

    dirname = os.path.join(medium_test_multiply_matrix_dir, name)
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


def randomly_permute(mat):
    """
    Randomly permute the rows and columns of a matrix.
    """
    np.random.seed(0)
    perm = matgen.generate_permutation_matrix(mat.get_shape()[0])
    return perm * mat * perm.transpose()


def generate_er_problems(scale, fill_factor=32, gitignored=True):
    """
    Generate a problem with two Erdos-Renyi matrices are multiplied together.
    """
    np.random.seed(scale)

    er = matgen.generate_er_matrix(2**scale, 2**scale, nnn=2**scale*fill_factor, dedupe=True)
    perm = matgen.generate_permutation_matrix(er.get_shape()[0])
    left_sub, right_sub = matgen.generate_submatrix_extraction(er.get_shape())

    generate_medium_test_multiply_problem(
        name=f"ER squared scale {scale}",
        a=er,
        b=er,
        gitignored=gitignored)

    generate_medium_test_multiply_problem(
        name=f"row perm of ER scale {scale}",
        a=perm,
        b=er,
        gitignored=gitignored)

    generate_medium_test_multiply_problem(
        name=f"submatrix of ER scale {scale}",
        a=left_sub,
        b=er,
        c=right_sub,
        gitignored=gitignored)


def generate_torus_problems(scale, gitignored=True):
    """
    Generate problems involving a 3D torus.
    """
    np.random.seed(scale)

    torus = matgen.generate_3d_torus_matrix(scale)
    perm = matgen.generate_permutation_matrix(torus.get_shape()[0])
    torus_rp = perm * torus * perm.transpose()

    generate_medium_test_multiply_problem(
        name=f"3D torus squared scale {scale}",
        a=torus,
        b=torus,
        gitignored=gitignored)

    generate_medium_test_multiply_problem(
        name=f"3D torus (rand perm) squared scale {scale}",
        a=torus_rp,
        b=torus_rp,
        gitignored=gitignored)

    generate_medium_test_multiply_problem(
        name=f"row perm of 3D torus scale {scale}",
        a=perm,
        b=torus,
        gitignored=gitignored)

    generate_medium_test_multiply_problem(
        name=f"row perm of 3D torus (RP) scale {scale}",
        a=perm,
        b=torus_rp,
        gitignored=gitignored)


def generate_medium_test_problems():
    """
    Generate problems for medium tests.

    These should be small enough to run in a second or two at most.
    """

    # tiny problems checked in to Git
    generate_er_problems(scale=5, gitignored=False)
    generate_torus_problems(scale=4, gitignored=False)

    # medium problems
    generate_er_problems(scale=10)

    for torus_scale in (20, 50,):
        generate_torus_problems(scale=torus_scale)
