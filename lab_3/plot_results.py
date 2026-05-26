"""
Построение графика времени MPI по результатам из result/statistics_mpi.txt.
Группирует точки по количеству процессов и строит зависимость времени
выполнения от размера матрицы.
"""
import re
from pathlib import Path

import matplotlib.pyplot as plt


STATS_FILE = Path("result/statistics_mpi.txt")
OUTPUT_FILE = Path("result/time_plot.png")


def parse_stats():
    text = STATS_FILE.read_text(encoding="utf-8")

    blocks = re.findall(
        r"Размер матрицы:\s*(\d+)x\d+\s*\n"
        r"Количество процессов:\s*(\d+)\s*\n"
        r"Время выполнения:\s*([\d.]+)\s*мс",
        text,
    )

    results = {}
    for size, procs, time_ms in blocks:
        size = int(size)
        procs = int(procs)
        time_sec = float(time_ms) / 1000.0
        results.setdefault(procs, []).append((size, time_sec))

    return results


def main():
    if not STATS_FILE.exists():
        raise FileNotFoundError(
            f"Файл {STATS_FILE} не найден. Сначала запустите main.exe через mpiexec."
        )

    results = parse_stats()

    plt.figure(figsize=(14, 7))

    for procs in sorted(results):
        points = sorted(results[procs])
        sizes = [p[0] for p in points]
        times = [p[1] for p in points]
        plt.plot(sizes, times, marker="o", linewidth=2,
                 label=f"{procs} процесс(а)")

    plt.title("Зависимость времени выполнения от размера матрицы и числа MPI-процессов",
              fontsize=16, fontweight="bold")
    plt.xlabel("Размер матрицы (N)", fontsize=14, fontweight="bold")
    plt.ylabel("Время (сек)", fontsize=14, fontweight="bold")
    plt.grid(True, linestyle="--", alpha=0.35)
    plt.legend(fontsize=12)
    plt.tight_layout()

    OUTPUT_FILE.parent.mkdir(exist_ok=True)
    plt.savefig(OUTPUT_FILE, dpi=200)
    print(f"График сохранён: {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
