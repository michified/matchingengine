import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


def read_csv_data(csv_path: Path):
    x = []
    series = {}

    with csv_path.open(newline="", encoding="utf-8") as csvfile:
        reader = csv.DictReader(csvfile)
        if not reader.fieldnames:
            raise ValueError(f"CSV file {csv_path} has no header row")

        numeric_columns = [name for name in reader.fieldnames if name != reader.fieldnames[0]]
        for name in numeric_columns:
            series[name] = []

        x_key = reader.fieldnames[0]
        for row in reader:
            try:
                x.append(float(row[x_key]))
            except (TypeError, ValueError):
                continue

            for name in numeric_columns:
                try:
                    series[name].append(float(row[name]))
                except (TypeError, ValueError, KeyError):
                    series[name].append(float("nan"))

    return x, series


def save_figure(fig, output_base: Path, suffix: str):
    output_base = output_base.with_suffix("")
    output_file = output_base.parent / f"{output_base.name}_{suffix}.png"
    output_file.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_file, dpi=150)
    print(f"Saved chart to {output_file}")


def plot_averagetimes(csv_path: Path, output_base: Path):
    x, series = read_csv_data(csv_path)
    if not x:
        raise ValueError(f"No valid data found in {csv_path}")

    primary_series = []
    percentile_series = []
    max_series = []
    other_cycles = []

    for name in series:
        lower_name = name.lower()
        if lower_name in ("runningavgns", "runningavgthroughput_mmsgsec"):
            primary_series.append(name)
        elif "median" in lower_name or "p95" in lower_name or "p999" in lower_name or "99.9" in lower_name:
            percentile_series.append(name)
        elif "max" in lower_name:
            max_series.append(name)
        else:
            other_cycles.append(name)

    plots = []

    # Latency and throughput chart
    fig, ax0 = plt.subplots(figsize=(12, 5))
    ax0.set_title("Latency and Throughput")
    if primary_series:
        ax1 = ax0.twinx()
        colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728"]
        for idx, name in enumerate(primary_series):
            target = ax0 if "ns" in name.lower() else ax1
            target.plot(x, series[name], marker="o", linestyle="-", color=colors[idx % len(colors)], label=name)

        ax0.set_ylabel("Latency")
        ax1.set_ylabel("Throughput (MMsg/sec)")
        ax0.grid(alpha=0.25)

        lines, labels = ax0.get_legend_handles_labels()
        lines2, labels2 = ax1.get_legend_handles_labels()
        ax0.legend(lines + lines2, labels + labels2, loc="upper left")
    else:
        ax0.text(0.5, 0.5, "No latency/throughput series found.", ha="center", va="center")
        ax0.set_ylabel("Value")

    fig.tight_layout(rect=[0, 0, 1, 0.96])
    plots.append((fig, "latency_throughput"))

    # Plot each percentile in its own chart
    percentile_colors = ["#9467bd", "#17becf", "#8c564b", "#e377c2", "#7f7f7f"]
    for idx, name in enumerate(percentile_series):
        fig, ax = plt.subplots(figsize=(12, 5))
        ax.set_title(name)
        ax.plot(x, series[name], marker="s", linestyle="-", color=percentile_colors[idx % len(percentile_colors)], label=name)
        ax.set_ylabel("Cycles")
        ax.set_xlabel("Tested Companies Amount")
        ax.legend(loc="upper left")
        ax.grid(alpha=0.25)
        fig.tight_layout(rect=[0, 0, 1, 0.96])
        safe_name = name.lower().replace(" ", "_").replace("/", "_")
        plots.append((fig, safe_name))

    # Plot max cycles chart
    if max_series:
        fig, ax_max = plt.subplots(figsize=(12, 5))
        ax_max.set_title("Max Cycles")
        for idx, name in enumerate(max_series):
            ax_max.plot(x, series[name], marker="^", linestyle="-", label=name, color="#d62728")
        ax_max.set_xlabel("Tested Companies Amount")
        ax_max.set_ylabel("Cycles")
        ax_max.legend(loc="upper left")
        ax_max.grid(alpha=0.25)
        fig.tight_layout(rect=[0, 0, 1, 0.96])
        plots.append((fig, "max_cycles"))

    # Plot any other cycles chart
    if other_cycles:
        fig, ax_other = plt.subplots(figsize=(12, 5))
        ax_other.set_title("Other Cycles")
        for idx, name in enumerate(other_cycles):
            ax_other.plot(x, series[name], marker="d", linestyle="-", label=name)
        ax_other.set_xlabel("Tested Companies Amount")
        ax_other.set_ylabel("Cycles")
        ax_other.legend(loc="upper left")
        ax_other.grid(alpha=0.25)
        fig.tight_layout(rect=[0, 0, 1, 0.96])
        plots.append((fig, "other_cycles"))

    for fig, suffix in plots:
        save_figure(fig, output_base, suffix)

    plt.show()


def main() -> None:
    script_root = Path(__file__).resolve().parent
    repo_root = script_root.parent
    default_csv = repo_root / "averagetimes.csv"
    default_output = repo_root / "plots" / "averagetimes_plot"

    parser = argparse.ArgumentParser(description="Plot averagetimes.csv data")
    parser.add_argument("--csv", type=Path, default=default_csv, help="Path to the CSV file")
    parser.add_argument("--output", type=Path, default=default_output, help="Output image path base")
    args = parser.parse_args()

    output_base = args.output.with_suffix("")
    output_base.parent.mkdir(parents=True, exist_ok=True)
    plot_averagetimes(args.csv, output_base)


if __name__ == "__main__":
    main()
