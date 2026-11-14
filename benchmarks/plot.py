import argparse
import csv
from collections import defaultdict
from pathlib import Path
from statistics import median

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def read_results(path):
    groups = defaultdict(list)
    repeats = defaultdict(set)
    sizes = {10000, 30000, 100000, 300000, 1000000}
    orders = {"ascending", "random"}
    operations = {"insert", "find_present", "find_absent", "erase"}
    with path.open(newline="") as source:
        for row in csv.DictReader(source):
            size = int(row["size"])
            order = row["order"]
            operation = row["operation"]
            repeat = int(row["repeat"])
            elapsed = float(row["time_ms"])
            expected = 0 if operation == "find_absent" else size
            key = (size, order, operation)
            if (size not in sizes or order not in orders or operation not in operations
                    or repeat not in range(1, 6) or repeat in repeats[key]
                    or int(row["seed"]) != 42 or not 0 < elapsed < float("inf")
                    or int(row["checksum"]) != expected):
                raise ValueError(f"Некорректный замер: {row}")
            repeats[key].add(repeat)
            groups[key].append(elapsed)

    if len(groups) != 40 or any(len(values) != 5 for values in groups.values()):
        raise ValueError("Ожидались 200 замеров: 5 размеров × 2 порядка × 4 операции × 5 повторов.")
    return {key: median(values) for key, values in groups.items()}


def plot_results(results, directory):
    directory.mkdir(parents=True, exist_ok=True)
    operations = {
        "insert": "Вставка",
        "find_present": "Успешный поиск",
        "find_absent": "Неуспешный поиск",
        "erase": "Удаление",
    }
    orders = {"ascending": "Возрастающий", "random": "Случайный"}
    sizes = sorted({key[0] for key in results})
    figure, axes = plt.subplots(2, 2, figsize=(12, 8), layout="constrained")
    for axis, (operation, title) in zip(axes.flat, operations.items()):
        for order, label in orders.items():
            times = [results[(size, order, operation)] for size in sizes]
            axis.plot(sizes, times, marker="o", label=label)
        axis.set_title(title)
        axis.set_xlabel("Количество элементов N")
        axis.set_ylabel("Время N операций, мс")
        axis.ticklabel_format(axis="x", style="sci", scilimits=(0, 0))
        axis.grid(alpha=0.3)
        axis.legend()
    figure.suptitle("AVL-дерево: медиана пяти повторов, Release")
    figure.savefig(directory / "timings.png", dpi=180)
    figure.savefig(directory / "timings.svg")
    plt.close(figure)

    with (directory / "medians.csv").open("w", newline="") as output:
        writer = csv.writer(output)
        writer.writerow(["size", "order", "operation", "median_ms"])
        for key, elapsed in sorted(results.items()):
            writer.writerow([*key, f"{elapsed:.6f}"])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path, nargs="?",
                        default=Path("benchmarks/results/raw.csv"))
    parser.add_argument("--output", type=Path, default=Path("benchmarks/results"))
    arguments = parser.parse_args()
    try:
        results = read_results(arguments.csv)
        plot_results(results, arguments.output)
    except (OSError, KeyError, ValueError) as error:
        parser.exit(1, f"Ошибка: {error}\n")


if __name__ == "__main__":
    main()
