"""
Построение графика времени CUDA-ядра по результатам из result/statistics_cuda.txt.
Группирует точки по размеру блока (8x8, 16x16, 32x32) и строит зависимость
времени выполнения от размера матрицы.
"""
import re
from pathlib import Path

import matplotlib.pyplot as plt


STATS_FILE = Path("result/statistics_cuda.txt")
OUTPUT_FILE = Path("result/time_plot.png")


def parse_stats():
    """Парсит файл статистики, возвращает dict[block_size] -> [(size, time_sec)]."""
    text = STATS_FILE.read_text(encoding="utf-8")

    blocks = re.findall(
        r"Размер матрицы:\s*(\d+)x\d+\s*\n"
        r"Конфигурация блока:\s*(\d+)x\d+\s*\n"
        r"Конфигурация сетки:\s*\d+x\d+\s*\n"
        r"Время выполнения ядра:\s*([\d.]+)\s*мс",
        text,
    )

    results = {}
    for size, block, time_ms in blocks:
        size = int(size)
        block = int(block)
        time_sec = float(time_ms) / 1000.0
        results.setdefault(block, []).append((size, time_sec))

    return results


def main():
    if not STATS_FILE.exists():
        raise FileNotFoundError(
            f"Файл {STATS_FILE} не найден. Сначала запустите main.exe."
        )

    results = parse_stats()

    plt.figure(figsize=(14, 7))

    for block in sorted(results):
        points = sorted(results[block])
        sizes = [p[0] for p in points]
        times = [p[1] for p in points]
        plt.plot(sizes, times, marker="o", linewidth=2,
                 label=f"блок {block}x{block}")

    plt.title("Зависимость времени CUDA-ядра от размера матрицы и размера блока",
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
