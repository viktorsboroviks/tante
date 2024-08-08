"""
Plot result.
"""

import argparse
import json
import math
import os
import re
import subprocess
import jsonschema
import natsort
import pandas as pd
import vplot
import vlog


CONFIG_SCHEMA_PATH = os.path.join(
    os.path.dirname(__file__),
    "plot_result_config.schema.json",
)


def plot_energy(config_json):
    input_csv_path = config_json["input_path"]
    output_graph_path = config_json["output_path"]

    try:
        energy_table = pd.read_csv(input_csv_path)
    except FileNotFoundError:
        print(f"plot_energy skipped: {input_csv_path} not found")
        return

    vplot.PlotlyPlot(
        subplots=[
            vplot.Subplot(
                col=1,
                row=1,
                traces=[
                    vplot.Step(
                        x=energy_table["state_i"],
                        y=energy_table["temperature"],
                        color=vplot.Color.RED,
                        name="temperature",
                    ),
                    vplot.Step(
                        x=energy_table["state_i"],
                        y=energy_table["energy"],
                        color=vplot.Color.BLUE,
                        name="energy",
                        # secondary_y=True,
                    ),
                ],
                x_min=energy_table["state_i"].iloc[0],
                x_max=energy_table["state_i"].iloc[-1],
            )
        ]
    ).to_file(output_graph_path)


def plot_account_data(config_json):
    font_size = config_json["font_size"]
    height = config_json["height"]
    width = config_json["width"]
    scale = config_json["scale"]
    log_path = config_json["log_path"]
    security_data_path = config_json["security_data_path"]
    security_name = config_json["security_name"]
    fx_data_path = config_json["fx_data_path"]
    security_name_in_account_currency = config_json["security_name_in_account_currency"]
    account_data_path_regex = config_json["account_data_path_regex"]
    operations_path_regex = config_json["operations_path_regex"]
    output_extension = config_json["output_extension"]

    _output = subprocess.check_output(
        ["find", ".", "-regex", account_data_path_regex], text=True
    )
    _account_data_paths = _output.strip().split("\n")
    sorted_account_data_paths = natsort.natsorted(_account_data_paths)

    _output = subprocess.check_output(
        ["find", ".", "-regex", operations_path_regex], text=True
    )
    operations_paths = _output.strip().split("\n")

    # read and compress energy table to save processing time
    try:
        energy_table = pd.read_csv(log_path)
        compressed_size = 1000
        compressed_x_step = int(energy_table.index.size / compressed_size)
        compressed_energy_table = energy_table.iloc[::compressed_x_step]
    except FileNotFoundError:
        print(f"plot_account_data skipped: {log_path} not found")
        return

    for i in range(len(sorted_account_data_paths)):
        account_data_path = sorted_account_data_paths[i]

        operations_path = re.sub(r"account_data", r"operations", account_data_path)
        assert operations_path in operations_paths

        output_graph_path = re.sub(r"csv$", output_extension, account_data_path)
        vlog.debug(
            f"generating {i+1}/{len(sorted_account_data_paths)}: {output_graph_path}"
        )

        if os.path.exists(output_graph_path):
            print("skipping: file already exists")
            continue

        # read operations
        try:
            # read metadata
            # - state
            # - max_open_positions
            # - n_assets
            with open(operations_path, "r") as f:
                for line in f:
                    if line.startswith("#") and ":" in line:
                        k, v = line[1:].split(":", 1)
                        k = k.strip()
                        if k == "state":
                            state = int(v)
                        elif k == "max_open_positions":
                            max_open_positions = int(v)
                        elif k == "n_assets":
                            n_assets = int(v)

            # read data
            operations_table = pd.read_csv(operations_path, comment="#")
        except FileNotFoundError:
            print(f"plot_operations skipped: {operations_path} not found")
            return

        # list of operation sequences
        # - skipped operations represented as lists of len=1
        # - later we will go over it to generate independent traces
        op_sequences = []
        processed_seq_ids = set()

        for _, operation in operations_table.iterrows():
            sim_seq_id = operation["sim_seq_id"]

            # extract skipped operations, as they form sequences of len=1
            if operation["sim_skipped"] == "SIM_SKIPPED":
                assert sim_seq_id not in processed_seq_ids
                op_sequences.append([operation])
                processed_seq_ids.add(sim_seq_id)
                continue
            assert operation["type"] != "OP_SKIP"

            # extract all remaining operations with the same sim_ep_seq_id
            if sim_seq_id not in processed_seq_ids:
                assert sim_seq_id not in processed_seq_ids
                filtered_table = operations_table[
                    operations_table["sim_seq_id"] == sim_seq_id
                ]
                op_sequence = []
                for _, filtered_op in filtered_table.iterrows():
                    op_sequence.append(filtered_op)
                op_sequences.append(op_sequence)
                processed_seq_ids.add(sim_seq_id)

        # sanity checks
        total_op_n = 0
        for ops in op_sequences:
            assert isinstance(ops, list)
            for op in ops:
                total_op_n += 1
                assert isinstance(op, pd.Series)
                assert op["sim_seq_id"] == ops[0]["sim_seq_id"]
                assert op["position_i"] == ops[0]["position_i"]
                if len(ops) > 1:
                    assert math.isnan(op["sim_skipped"])

        assert total_op_n == len(operations_table)

        # generate position traces
        def _position_op_trace(ops) -> tuple[int, vplot.Scatter]:
            if ops[0]["sim_skipped"] == "SIM_SKIPPED":
                assert len(ops) == 1
                color = vplot.Color.LIGHT_GREY
            else:
                color = vplot.Color.BLUE
            x_values = []
            y_values = []
            for op in ops:
                x_values.append(op["dt"])
                if op["type"] == "OP_SKIP":
                    y_values.append(0.0)
                elif op["type"] == "OP_BUY_ALL":
                    y_values.append(1.0)
                elif op["type"] == "OP_BUY_FRACTION":
                    y_values.append(op["fraction"])
                elif op["type"] == "OP_SELL_ALL":
                    y_values.append(-1.0)
                elif op["type"] == "OP_SELL_FRACTION":
                    y_values.append(-op["fraction"])
            trace = [
                vplot.Scatter(
                    x=x_values,
                    y=y_values,
                    color=color,
                    width=0.5,
                    mode="lines+markers",
                    marker_symbol=vplot.MarkerSymbol.CIRCLE,
                    marker_size=3,
                )
            ]
            return ops[0]["position_i"], trace

        # generate traces for skipped ops first, so they end up
        # in the background relative to the open positions
        position_op_traces = [[] for _ in range(max_open_positions)]

        for ops in op_sequences:
            if ops[0]["sim_skipped"] == "SIM_SKIPPED":
                position_i, trace = _position_op_trace(ops)
                position_op_traces[position_i] += trace

        for ops in op_sequences:
            if ops[0]["sim_skipped"] != "SIM_SKIPPED":
                position_i, trace = _position_op_trace(ops)
                position_op_traces[position_i] += trace

        # generate price position traces
        def _position_price_trace(ops) -> tuple[int, vplot.Scatter]:
            assert ops[0]["sim_skipped"] != "SIM_SKIPPED"

            x_values = []
            y_values = []
            for op in ops:
                assert op["type"] != "OP_SKIP"
                x_values.append(op["dt"])
                y_series = security_in_account_currency.loc[
                    security_in_account_currency["Date"] == op["dt"], "Close"
                ]
                assert y_series.size == 1
                y_values.append(y_series.iloc[0])

            # mark profitable trades as green
            # TODO: improve the calculation, current one if very rude
            if y_values[0] < y_values[-1]:
                color = vplot.Color.GREEN
            else:
                color = vplot.Color.RED
            trace = [
                vplot.Scatter(
                    x=x_values,
                    y=y_values,
                    color=color,
                    width=0.5,
                    mode="lines+markers",
                    marker_symbol=vplot.MarkerSymbol.CIRCLE,
                    marker_size=3,
                )
            ]
            return ops[0]["position_i"], trace

        # read price data and generate price traces
        price_traces = []
        # security data currency matches the account currency
        try:
            security_in_account_currency = pd.read_csv(security_data_path)
        except FileNotFoundError:
            print(f"plot_account_data skipped: {security_data_path} not found")
            return

        price_traces += [
            vplot.Step(
                x=security_in_account_currency["Date"],
                y=security_in_account_currency["Close"],
                color=vplot.Color.LIGHT_GREY,
                name=security_name,
                showlegend=True,
            )
        ]

        for ops in op_sequences:
            if ops[0]["sim_skipped"] != "SIM_SKIPPED":
                _, trace = _position_price_trace(ops)
                price_traces += trace

        # generate profit position traces
        def _position_profit_trace(ops) -> tuple[int, vplot.Scatter]:
            assert ops[0]["sim_skipped"] != "SIM_SKIPPED"

            x_values = []
            y_values = []
            for op in ops:
                x_values.append(op["dt"])
                profit = (
                    op["sim_pos_value"]
                    + op["sim_pos_total_gain"]
                    - op["sim_pos_total_cost"]
                ) / op["sim_pos_total_cost"]
                y_values.append(profit)

            # mark profitable trades as green
            if y_values[0] < y_values[-1]:
                color = vplot.Color.GREEN
            else:
                color = vplot.Color.RED
            trace = [
                vplot.Scatter(
                    x=x_values,
                    y=y_values,
                    color=color,
                    width=0.5,
                    mode="lines+markers",
                    marker_symbol=vplot.MarkerSymbol.CIRCLE,
                    marker_size=3,
                )
            ]
            return ops[0]["position_i"], trace

        profit_traces = []
        for ops in op_sequences:
            if ops[0]["sim_skipped"] != "SIM_SKIPPED":
                _, trace = _position_profit_trace(ops)
                profit_traces += trace

        # read account data
        try:
            # read metadata
            # - state
            # - n_states
            # - energy
            with open(account_data_path, "r") as f:
                for line in f:
                    if line.startswith("#") and ":" in line:
                        k, v = line[1:].split(":", 1)
                        k = k.strip()
                        if k == "state_i":
                            _state = int(v)
                            assert _state == state
                        elif k == "n_states":
                            n_states = int(v)
                        elif k == "energy":
                            fitness = float(v)

            # read data
            account_data_table = pd.read_csv(account_data_path, comment="#")
        except FileNotFoundError:
            print(f"plot_account_data skipped: {account_data_path} not found")
            return

        # generate subplots
        subplots = []
        last_row = 1
        row_ratios = []
        col_ratios = [0.9, 0.1]
        x_dt_min = account_data_table["date"].iloc[0]
        x_dt_max = account_data_table["date"].iloc[-1]

        for _ in range(max_open_positions):
            row_ratios += [0.4 / max_open_positions]
        for i in range(max_open_positions):
            subplots += [
                vplot.Subplot(
                    col=1,
                    row=last_row,
                    traces=position_op_traces[i],
                    x_min=x_dt_min,
                    x_max=x_dt_max,
                    subtitle_text=f"operations position {i}",
                    subtitle_x=-0.002,
                    subtitle_y=-0.33,
                )
            ]
            last_row += 1

        row_ratios += [0.3]
        subplots += [
            vplot.Subplot(
                col=1,
                row=last_row,
                traces=profit_traces,
                x_min=x_dt_min,
                x_max=x_dt_max,
                subtitle_text="(value+profit)/cost",
                subtitle_x=-0.004,
                subtitle_y=-0.15,
                legendgroup_name="profit",
            ),
        ]
        last_row += 1

        row_ratios += [0.3]
        subplots += [
            vplot.Subplot(
                col=1,
                row=last_row,
                traces=price_traces,
                x_min=x_dt_min,
                x_max=x_dt_max,
                subtitle_text="trades",
                subtitle_x=-0.004,
                subtitle_y=-0.15,
                legendgroup_name="trades",
            ),
        ]
        last_row += 1

        row_ratios += [0.3]
        subplots += [
            vplot.Subplot(
                col=1,
                row=last_row,
                subtitle_text="total",
                subtitle_x=-0.004,
                subtitle_y=-0.17,
                legendgroup_name="total",
                traces=[
                    vplot.Step(
                        x=account_data_table["date"],
                        y=account_data_table["total_value"],
                        color=vplot.Color.LIGHT_GREY,
                        name="value",
                        showlegend=True,
                    ),
                    vplot.Step(
                        x=account_data_table["date"],
                        y=account_data_table["total_fees"],
                        color=vplot.Color.RED,
                        name="fees",
                        showlegend=True,
                    ),
                    vplot.Step(
                        x=account_data_table["date"],
                        y=account_data_table["cash"],
                        color=vplot.Color.GREEN,
                        name="cash",
                        showlegend=True,
                    ),
                ],
                x_min=x_dt_min,
                x_max=x_dt_max,
            ),
        ]

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

        vplot.PlotlyPlot(
            title_text=f"gen: {state}/{n_states}, fitness: {fitness}",
            font_size=font_size,
            height=height,
            width=width,
            col_ratios=col_ratios,
            row_ratios=row_ratios,
            subplots=subplots,
        ).to_file(output_graph_path, scale=scale)


vlog.configure("debug")

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("config", help="path to a .json config file")
args = parser.parse_args()

with open(CONFIG_SCHEMA_PATH) as f:
    config_schema = json.load(f)
with open(args.config) as f:
    config_json = json.load(f)["plot_result"]

jsonschema.validate(instance=args.config, schema=config_schema)

plot_energy(config_json["energy_graph"])
plot_account_data(config_json["account_data_graph"])
