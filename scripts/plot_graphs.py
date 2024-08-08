import argparse
import json
import natsort
import subprocess


DEFAULT_CONFIG_SECTION = "plot_graph"


def find_by_prefix(search_dir, prefix):
    regex = search_dir + "/" + prefix + ".*"
    paths = (
        subprocess.check_output(["find", search_dir, "-regex", regex], text=True)
        .strip()
        .split("\n")
    )
    assert len(paths) > 0
    return natsort.natsorted(paths)


def get_file_i_str(filepath):
    return filepath[filepath.find("_") + 1 : filepath.rfind(".csv")]


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

# find all files
vertices_regex = config_json["search_dir"] + "/" + config_json["vertices_prefix"] + ".*"
vertices_paths = (
    subprocess.check_output(
        ["find", config_json["search_dir"], "-regex", vertices_regex], text=True
    )
    .strip()
    .split("\n")
)

assert len(vertices_paths) > 0

vertices_paths = find_by_prefix(
    config_json["search_dir"], config_json["vertices_prefix"]
)
edges_paths = find_by_prefix(config_json["search_dir"], config_json["edges_prefix"])
assert len(vertices_paths) == len(edges_paths)

for i in range(len(vertices_paths)):
    vertices_path = vertices_paths[i]
    edges_path = edges_paths[i]
    assert get_file_i_str(vertices_path) == get_file_i_str(edges_path)
    output_path = (
        config_json["output_dir"]
        + "/"
        + config_json["output_prefix"]
        + get_file_i_str(vertices_path)
        + ".svg"
    )

    print(f"[{i}/{len(vertices_paths)}] generating graph file {output_path}")
    subprocess.run(
        [
            "python3",
            config_json["plot_graph_path"],
            f"--config={args.config}",
            f"--config-section=plot_graph",
            f"--vertices={vertices_path}",
            f"--edges={edges_path}",
            f"--output={output_path}",
        ]
    )
