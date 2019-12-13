import numpy as np
from scipy.sparse import csc_matrix
import scipy.io


def generate_torus_matrix(dim, n_axis):
    """
    Constructs an adjacency matrix representation of a torus graph of dimension `dim`.
    Each vertex has edges to:
     - itself (0D)
     - its east and west (1D) neighbors
     - its north and south (2D) neighbors
     - its front and back (3D) neighbors
     - etc.
    Unlike a grid, torus neighbor edges may "wrap around".

    @param dim dimensionality of the torus.
    @type dim int
    @param n_axis the number of nodes on each axis of the torus. The torus will have n_axis**`dim` vertices.
    @type n_axis int
    @returns a scipy.sparse matrix
    """
    n = n_axis**dim
    rows = np.arange(0, n)
    eye_cols = np.arange(0, n)

    # Neighbor links are created by offsetting a copy of the main diagonal. Find these offsets.
    col_offsets = [0] if dim == 0 else [0, 1, -1]
    for d in range(1, dim):
        col_offsets.append((n_axis**d-1))
        col_offsets.append(-(n_axis**d-1))

    # Create offsets diagonals.
    col_chunks = []
    for offset in col_offsets:
        cols = (eye_cols + offset) % n
        col_chunks.append(cols)

    # Concatenate all chunks into the final matrix tuples
    cols = np.concatenate(col_chunks)
    rows = np.concatenate((rows,) * len(col_offsets))
    vals = np.ones(n * len(col_offsets))

    return scipy.sparse.csc_matrix((vals, (rows, cols)), shape=(n, n))


def generate_2d_torus_matrix(n_axis):
    """
    Constructs an adjacency matrix representation of a 2D torus graph.
    Each vertex has edges to itself and its north, west, south, and east neighbors.
    Unlike a grid, torus neighbor edges may "wrap around".

    @param n_axis the number of nodes on each axis of the torus. A 2D torus will have n_axis**2 vertices.
    @type n_axis int
    @returns a scipy.sparse matrix
    """
    return generate_torus_matrix(dim=2, n_axis=n_axis)


def generate_3d_torus_matrix(n_axis):
    """
    Constructs an adjacency matrix representation of a 3D torus graph.
    Each vertex has edges to itself and its north, west, south, east, front, and back neighbors.
    Unlike a grid, torus neighbor edges may "wrap around".

    @param n_axis the number of nodes on each axis of the torus. A 3D torus will have n_axis**3 vertices.
    @type n_axis int
    @returns a scipy.sparse matrix
    """
    return generate_torus_matrix(dim=3, n_axis=n_axis)


def generate_er_matrix(m, n, nnn, dedupe=True):
    """
    Generate an Erdos-Renyi matrix.

    @param m number of rows
    @type m int
    @param n number of columns
    @type n int
    @param nnn number of non-nulls
    @type nnn int
    @param dedupe if False then there may be multiple entries with the same row, column index

    @returns a scipy.sparse matrix
    """
    rows = np.random.randint(low=0, high=m, size=nnn)
    cols = np.random.randint(low=0, high=n, size=nnn)
    vals = np.ones(nnn)

    ret = scipy.sparse.csc_matrix((vals, (rows, cols)), shape=(m, n))

    if dedupe:
        ret.sum_duplicates()
        ret.data.fill(1)

    return ret


def generate_permutation_matrix(n):
    """
    Generate a permutation matrix where the rows of an identity matrix are randomly shuffled.

    @param n number of rows and columns in the matrix
    @type n int

    @returns a scipy.sparse matrix
    """
    rows = np.random.permutation(n)
    cols = np.arange(n)
    vals = np.ones(n)

    return scipy.sparse.csc_matrix((vals, (rows, cols)), shape=(n, n))


def generate_submatrix_extraction(shape, divisor=2):
    """
    Generate left and right matrices to perform a submatrix extraction. The columns to extract are selected evenly.
    """
    if shape[0] != shape[1]:
        raise ValueError("only square matrices supported")

    n = shape[0]
    k = int(n/divisor)

    vals = np.ones(k)
    left_rows = np.arange(k)
    left_cols = left_rows * divisor
    left = scipy.sparse.csc_matrix((vals, (left_rows, left_cols)), shape=(k, n))

    right = scipy.sparse.csc_matrix((vals, (left_cols, left_rows)), shape=(n, k))

    return left, right

