#!/usr/bin/env python3
"""
Generate problems for QuadMat unit and medium tests.
"""

import argparse

import gen.multiply_problem_generators as mpg
from gen import unit_test_multiply_matrix_dir, medium_test_multiply_matrix_dir


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('type', choices=["unit", "medium"], default="medium", nargs='?',
                        help="Which tests to generate: "
                             "unit (these are small and checked in to Git) or "
                             "medium (too large for Git)")

    ret = parser.parse_args()
    ret.unit = (ret.type == "unit")
    ret.medium = (ret.type == "medium")
    return ret


def generate_unit_test_problems():
    """
    Generate problems for unit tests.

    These need to be small in order to run in milliseconds with unoptimized code.
    """
    # multiply
    mpg.generate_kg_square_problem(basedir=unit_test_multiply_matrix_dir)


def generate_medium_test_problems():
    """
    Generate problems for medium tests.

    These should be small enough to run in a second or two at most.
    """

    for er_scale in (12, 14,):
        mpg.generate_er_problems(basedir=medium_test_multiply_matrix_dir, scale=er_scale)

    for torus_scale in (25, 50,):
        mpg.generate_torus_problems(basedir=medium_test_multiply_matrix_dir, scale=torus_scale)


def main():
    args = parse_args()

    if args.unit:
        print("Generating unit test problems to " + unit_test_multiply_matrix_dir)
        generate_unit_test_problems()

    if args.medium:
        print("Generating medium test problems to " + medium_test_multiply_matrix_dir)
        generate_medium_test_problems()


if __name__ == "__main__":
    main()
