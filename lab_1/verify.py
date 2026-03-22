import numpy as np
import os
import sys

SIZES = [200, 400, 800, 1200, 1600, 2000]
MATRIX_DIR = "matrix"
RESULT_DIR = "result"

def load_matrix(filepath):
    try:
        return np.loadtxt(filepath, dtype=np.int64)
    except Exception:
        return None

def verify_result(size):
    file_first = os.path.join(MATRIX_DIR, f"first_{size}.txt")
    file_second = os.path.join(MATRIX_DIR, f"second_{size}.txt")
    file_result = os.path.join(RESULT_DIR, f"result_{size}.txt")

    first_matrix = load_matrix(file_first)
    second_matrix = load_matrix(file_second)
    result_matrix = load_matrix(file_result)

    if first_matrix is None or second_matrix is None or result_matrix is None:
        return False

    computed_matrix = np.dot(first_matrix, second_matrix)

    if np.array_equal(result_matrix, computed_matrix):
        return True
    else:
        return False

def main():
    if not os.path.exists(MATRIX_DIR) or not os.path.exists(RESULT_DIR):
        print("Directories not found")
        sys.exit(1)

    all_passed = True
    for size in SIZES:
        is_correct = verify_result(size)
        status = "Passed" if is_correct else "Failed"
        print(f"Size {size}: {status}")
        if not is_correct:
            all_passed = False

    if all_passed:
        print("All tests passed")
        sys.exit(0)
    else:
        print("Errors detected")
        sys.exit(1)

if __name__ == "__main__":
    main()