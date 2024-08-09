import argparse
import json
import pandas as pd
import vplot


DEFAULT_CONFIG_SECTION = "plot_all"
COMPRESSED_SIZE = 1000


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--config", help="path to a .json config file")
parser.add_argument(
    "--config-section",
    help="name of the section in config",
    default=DEFAULT_CONFIG_SECTION,
    required=False,
)
args = parser.parse_args()

with open(args.config) as f:
    config_json = json.load(f)
if args.config_section:
    config_json = config_json[args.config_section]

# read and compress energy table to save processing time
log_path = config_json["log_path"]
try:
    energy_table = pd.read_csv(log_path)
    compressed_size = COMPRESSED_SIZE
    if (energy_table.index.size > compressed_size):
        compressed_x_step = int(energy_table.index.size / compressed_size)
    else:
        compressed_x_step = 1
    compressed_energy_table = energy_table.iloc[::compressed_x_step]
except FileNotFoundError:
    print(f"plot_account_data skipped: {log_path} not found")
    exit(-1)

# energy subplot
subplots += [
    vplot.Subplot(
        col=2,
        row=[1, last_row],
        subtitle_text="energy",
        subtitle_x=-0.03,
        subtitle_y=-0.07,
        traces=[
            vplot.Step(
                x=compressed_energy_table["state_i"],
                y=compressed_energy_table["temperature"],
                color=vplot.Color.RED,
                name="temperature",
                showlegend=True,
            ),
            vplot.Step(
                x=compressed_energy_table["state_i"],
                y=compressed_energy_table["energy"],
                # secondary_y=True,
                color=vplot.Color.BLUE,
                name="energy",
                showlegend=True,
            ),
        ],
        lines=[
            vplot.Lines(
                x=[state],
                color=vplot.Color.RED,
                dash=vplot.Dash.SOLID,
            )
        ],
    )
]
