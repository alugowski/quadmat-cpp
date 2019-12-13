#!/usr/bin/env python3
"""
Script to generate problems for QuadMat unit and medium tests.
"""

from gen.unit_test_generators import generate_unit_test_problems
from gen.medium_test_generators import generate_medium_test_problems


def main():
    generate_unit_test_problems()

    generate_medium_test_problems()


if __name__ == "__main__":
    main()
