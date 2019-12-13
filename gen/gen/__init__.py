import os.path

test_matrix_dir = os.path.join("..", "test", "matrices")
"""
Directory where to place generated matrices.
"""

unit_test_matrix_dir = os.path.join(test_matrix_dir, "unit")
unit_test_multiply_matrix_dir = os.path.join(unit_test_matrix_dir, "multiply")

medium_test_multiply_matrix_dir = os.path.join(test_matrix_dir, "medium", "multiply")
