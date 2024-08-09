import argparse
import json
import pandas as pd
import vplot


DEFAULT_CONFIG_SECTION = "plot_all"
COMPRESSED_SIZE = 1000

def inference_subplot(
    data_csv,
    col,
    row,
    data_col_x,
    data_col_out,
    data_col_cor,
    compress_x_to_n=COMPRESSED_SIZE
):
    try:
        table = pd.read_csv(data_csv, comment="#")
        if table.index.size > compress_x_to_n:
            x_step = int(table.index.size / compress_x_to_n)
        else:
            x_step = 1
        table = table.iloc[::x_step]
    except FileNotFoundError:
        raise FileNotFoundError(f"{data_csv} not found")

    return vplot.Subplot(
        col=col,
        row=row,
        traces=[
            vplot.Scatter(
                mode="markers",
                x=table[data_col_x],
                y=table[data_col_out],
                color=vplot.Color.RED,
                name="output",
                showlegend=True,
            ),
            vplot.Scatter(
                mode="markers",
                x=table[data_col_x],
                y=table[data_col_cor],
                color=vplot.Color.GREY,
                name="correct",
                showlegend=True,
            ),
        ],
    )


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

subplot = inference_subplot(data_csv=config_json["data_csv"],
                            col=1,
                            row=1,
                            data_col_x="signal_input",
                            data_col_out="signal_output",
                            data_col_cor="signal_correct")

vplot.PlotlyPlot(
    title_text="infered sin(x)",
    subplots=[subplot]
).to_file(config_json["output_svg"])
