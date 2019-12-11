import numpy as np
from scipy.sparse import csc_matrix
import scipy.io


def generate_2d_torus_matrix(n):
    pass


def generate_er_matrix(n: int, m: int, nnn: int, dedupe=True):
    """
    Generate an Erdos-Renyi matrix.

    @param n number of columns
    @param m number of rows
    @param nnn number of non-nulls
    @param dedupe if False then there may be multiple entries with the same row, column index

    @returns a scipy.sparse.csc_matrix
    """
    rows = np.random.randint(low=0, high=m, size=nnn)
    cols = np.random.randint(low=0, high=n, size=nnn)
    vals = np.ones(nnn)

    ret = scipy.sparse.csc_matrix((vals, (rows, cols)), shape=(m, n))

    if dedupe:
        ret.sum_duplicates()
        ret.data.fill(1)

    return ret
