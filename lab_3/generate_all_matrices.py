"""
Генерация входных матриц для лабораторной работы №3 (MPI).
Формат: целые числа 0-99, через пробелы, без заголовка с размером.
Файлы сохраняются в matrix/ как first_<N>.txt и second_<N>.txt.
"""
import os
import numpy as np

SIZES = [200, 400, 800, 1200, 1600, 2000]
MATRIX_DIR = "matrix"


def save_matrix(matrix, filename):
    np.savetxt(filename, matrix, fmt="%4d")


def generate(size, rng):
    A = rng.integers(0, 100, size=(size, size), dtype=np.int64)
    B = rng.integers(0, 100, size=(size, size), dtype=np.int64)

    save_matrix(A, os.path.join(MATRIX_DIR, f"first_{size}.txt"))
    save_matrix(B, os.path.join(MATRIX_DIR, f"second_{size}.txt"))

    print(f"  {size}x{size}: first_{size}.txt, second_{size}.txt")


def main():
    print("=" * 50)
    print("Генерация входных матриц")
    print("=" * 50)

    os.makedirs(MATRIX_DIR, exist_ok=True)
    rng = np.random.default_rng(42)

    for size in SIZES:
        generate(size, rng)

    print("-" * 50)
    print(f"Готово. Файлы лежат в {MATRIX_DIR}/")


if __name__ == "__main__":
    main()
