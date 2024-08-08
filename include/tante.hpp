#include <array>
#include <cassert>
#include <functional>
#include <map>
#include <optional>
#include <queue>
#include <vector>

// #define PRINT_DEBUGS
#ifdef PRINT_DEBUGS
#define DEBUG(x)                                  \
    do {                                          \
        std::cout << "debug: " << x << std::endl; \
    } while (0)
#else
#define DEBUG(x)
#endif

#include "garaza.hpp"
#include "grafiins.hpp"
#include "iestade.hpp"
#include "rododendrs.hpp"

namespace tante {

enum Operation {
    ADD_INPUT = 0,
    RM_INPUT,
    ADD_OUTPUT,
    RM_OUTPUT,
    ADD_HIDDEN,
    RM_HIDDEN,
    ADD_CONNECTION,
    RM_CONNECTION,
    STEP_WEIGHT,
    STEP_BIAS,
    RND_WEIGHT,
    RND_BIAS,
    N_OPS,
};

class Neuron : public grafiins::Vertex {
public:
    enum AFID {
        AF_RANDOM = -1,
        AF_TANH   = 0,
        AF_SIGMOID,
        AF_RELU,
        N_AFS,
    };

    AFID afid;
    double bias = 0;

    Neuron(AFID afid = AF_SIGMOID, std::string label = "") :
        Vertex(label),
        afid(afid)
    {
    }

    static double af_tanh(double in)
    {
        return std::tanh(in);
    }

    static double af_sigmoid(double in)
    {
        return 1 / (1 + std::exp(-in));
    }

    static double af_relu(double in)
    {
        return std::max(0.0, in);
    }

    double activation_f(double in) const
    {
        return _activation_f_by_id(in, afid);
    }

    static std::string afid_to_str(AFID in)
    {
        switch (in) {
            case AFID::AF_RANDOM:
                return "random";
            case AFID::AF_TANH:
                return "tanh";
            case AFID::AF_SIGMOID:
                return "sigmoid";
            case AFID::AF_RELU:
                return "relu";
            default:
                // this should not happen
                assert(false);
                break;
        }
        // this should not happen
        assert(false);
        return "";
    }

    static AFID str_to_afid(const std::string& in)
    {
        if (in == "random") {
            return AFID::AF_RANDOM;
        }
        else if (in == "tanh") {
            return AFID::AF_TANH;
        }
        else if (in == "sigmoid") {
            return AFID::AF_SIGMOID;
        }
        else if (in == "relu") {
            return AFID::AF_RELU;
        }

        // this should not happen
        assert(false);
        return AFID::AF_SIGMOID;
    }

    std::map<std::string, std::string> serialize()
    {
        std::map<std::string, std::string> m = grafiins::Vertex::serialize();
        m["activation_function"]             = afid_to_str(afid);
        m["bias"]                            = std::to_string(bias);
        return m;
    }

private:
    double _activation_f_by_id(double in, AFID in_afid) const
    {
        const int rnd_afid = rododendrs::rnd01() * (double)N_AFS;
        switch (in_afid) {
            case AF_RANDOM:
                return _activation_f_by_id(in, (AFID)rnd_afid);
            case AF_TANH:
                return af_tanh(in);
            case AF_SIGMOID:
                return af_sigmoid(in);
            case AF_RELU:
                return af_relu(in);
            case N_AFS:
            default:
                assert(false);
                break;
        }

        assert(false);
        return -1.0;
    }
};

class Connection : public grafiins::Edge {
public:
    double weight;

    Connection(size_t src_i,
               size_t dst_i,
               double weight,
               std::string label = "") :
        Edge(src_i, dst_i, label),
        weight(weight)
    {
    }

    Connection() :
        Connection(0, 0, 0.0)
    {
    }

    std::map<std::string, std::string> serialize()
    {
        std::map<std::string, std::string> m = grafiins::Edge::serialize();

        m["weight"] = std::to_string(weight);
        return m;
    }
};

struct Settings {
    size_t n_inputs                     = 1;
    size_t n_outputs                    = 1;
    size_t max_n_hidden                 = 10;
    double min_init_weight              = -10;
    double max_init_weight              = 10;
    bool limit_weight                   = false;
    bool limit_bias                     = false;
    double min_weight                   = -100;
    double max_weight                   = 100;
    double min_bias                     = -100;
    double max_bias                     = 100;
    double min_weight_step              = -10;
    double max_weight_step              = 10;
    double min_bias_step                = -10;
    double max_bias_step                = 10;
    size_t op_weights[Operation::N_OPS] = {1};
    Neuron::AFID neuron_afid            = Neuron::AFID::AF_SIGMOID;
    std::string input_graphviz_shape    = "doublecircle";
    std::string input_graphviz_cluster  = "inputs";
    double input_graphviz_width         = 0.4;
    std::string output_graphviz_shape   = "doublecircle";
    std::string output_graphviz_cluster = "outputs";
    double output_graphviz_width        = 0.4;
    std::string hidden_graphviz_shape   = "circle";
    std::string hidden_graphviz_cluster = "";
    double hidden_graphviz_width        = 0.4;

    Settings() {}

    // clang-format off
    Settings(const std::string& config_filepath,
             const std::string& key_path_prefix) :
        n_inputs                (iestade::size_t_from_json(config_filepath, key_path_prefix + "/n_inputs")),
        n_outputs               (iestade::size_t_from_json(config_filepath, key_path_prefix + "/n_outputs")),
        max_n_hidden            (iestade::size_t_from_json(config_filepath, key_path_prefix + "/max_n_hidden")),
        min_init_weight         (iestade::double_from_json(config_filepath, key_path_prefix + "/min_init_weight")),
        max_init_weight         (iestade::double_from_json(config_filepath, key_path_prefix + "/max_init_weight")),
        limit_weight            (iestade::bool_from_json  (config_filepath, key_path_prefix + "/limit_weight")),
        limit_bias              (iestade::bool_from_json  (config_filepath, key_path_prefix + "/limit_bias")),
        min_weight              (iestade::double_from_json(config_filepath, key_path_prefix + "/min_weight")),
        max_weight              (iestade::double_from_json(config_filepath, key_path_prefix + "/max_weight")),
        min_bias                (iestade::double_from_json(config_filepath, key_path_prefix + "/min_bias")),
        max_bias                (iestade::double_from_json(config_filepath, key_path_prefix + "/max_bias")),
        min_weight_step         (iestade::double_from_json(config_filepath, key_path_prefix + "/min_weight_step")),
        max_weight_step         (iestade::double_from_json(config_filepath, key_path_prefix + "/max_weight_step")),
        min_bias_step           (iestade::double_from_json(config_filepath, key_path_prefix + "/min_bias_step")),
        max_bias_step           (iestade::double_from_json(config_filepath, key_path_prefix + "/max_bias_step")),
        neuron_afid             (Neuron::str_to_afid(iestade::string_from_json(config_filepath, key_path_prefix + "/neuron_activation_function"))),
        input_graphviz_shape    (iestade::string_from_json(config_filepath, key_path_prefix + "/graphviz/input_shape")),
        input_graphviz_cluster  (iestade::string_from_json(config_filepath, key_path_prefix + "/graphviz/input_cluster")),
        input_graphviz_width    (iestade::double_from_json(config_filepath, key_path_prefix + "/graphviz/input_width")),
        output_graphviz_shape   (iestade::string_from_json(config_filepath, key_path_prefix + "/graphviz/output_shape")),
        output_graphviz_cluster (iestade::string_from_json(config_filepath, key_path_prefix + "/graphviz/output_cluster")),
        output_graphviz_width   (iestade::double_from_json(config_filepath, key_path_prefix + "/graphviz/output_width")),
        hidden_graphviz_shape   (iestade::string_from_json(config_filepath, key_path_prefix + "/graphviz/hidden_shape")),
        hidden_graphviz_cluster (iestade::string_from_json(config_filepath, key_path_prefix + "/graphviz/hidden_cluster")),
        hidden_graphviz_width   (iestade::double_from_json(config_filepath, key_path_prefix + "/graphviz/hidden_width"))
    {
        op_weights[Operation::ADD_INPUT]        = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/add_input");
        op_weights[Operation::RM_INPUT]         = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/rm_input");
        op_weights[Operation::ADD_OUTPUT]       = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/add_output");
        op_weights[Operation::RM_OUTPUT]        = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/rm_output");
        op_weights[Operation::ADD_HIDDEN]       = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/add_hidden");
        op_weights[Operation::RM_HIDDEN]        = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/rm_hidden");
        op_weights[Operation::ADD_CONNECTION]   = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/add_connection");
        op_weights[Operation::RM_CONNECTION]    = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/rm_connection");
        op_weights[Operation::STEP_WEIGHT]      = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/step_weight");
        op_weights[Operation::STEP_BIAS]        = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/step_bias");
        op_weights[Operation::RND_WEIGHT]       = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/rnd_weight");
        op_weights[Operation::RND_BIAS]         = iestade::size_t_from_json(config_filepath, key_path_prefix + "/op_weights/rnd_bias");
    }
    // clang-format on
};

class Network {
public:
    Settings settings;

    Network(Settings& in_settings) :
        settings(in_settings)
    {
        assert(settings.n_inputs > 0);
        assert(settings.n_outputs > 0);
        assert(settings.max_n_hidden > 0);
        assert(settings.min_init_weight <= settings.max_init_weight);
        assert(settings.min_weight_step <= settings.max_weight_step);
        assert(settings.min_bias_step <= settings.max_bias_step);
    }

    bool is_operational()
    {
        DEBUG("checking if operational...");

        assert(_inputs_i.size() <= settings.n_inputs);
        assert(_outputs_i.size() <= settings.n_outputs);
        assert(_hidden_i.size() <= settings.max_n_hidden);

        if (_inputs_i.empty()) {
            DEBUG("no inputs.");
            return false;
        }

        if (_outputs_i.empty()) {
            DEBUG("no outputs.");
            return false;
        }

        const std::list<size_t> list_inputs_vi  = _inputs_i.list();
        const std::list<size_t> list_outputs_vi = _outputs_i.list();
        const std::set<size_t> set_inputs_vi{list_inputs_vi.begin(),
                                             list_inputs_vi.end()};
        const std::set<size_t> set_outputs_vi{list_outputs_vi.begin(),
                                              list_outputs_vi.end()};

        // every input has a connection to at least one output
        for (size_t ivi : list_inputs_vi) {
            if (!_g.are_connected_any({ivi}, set_outputs_vi)) {
                DEBUG("disconnected input found.");
                return false;
            }
        }

        // every output has a connection to at least one input
        for (size_t ovi : list_outputs_vi) {
            if (!_g.are_connected_any(set_inputs_vi, {ovi})) {
                DEBUG("disconnected output found.");
                return false;
            }
        }

        return true;
    }

    // keep applying random operations until the network becomes operational
    void restore_randomly()
    {
        // add missing inputs
        const size_t add_n_inputs = settings.n_inputs - _inputs_i.size();
        for (size_t i = 0; i < add_n_inputs; i++) {
            _add_input();
        }
        assert(_inputs_i.size() == settings.n_inputs);

        // add missing outputs
        const size_t add_n_outputs = settings.n_outputs - _outputs_i.size();
        for (size_t i = 0; i < add_n_outputs; i++) {
            _add_output();
        }
        assert(_outputs_i.size() == settings.n_outputs);

        // add connections and hidden neurons until the network is restored
        while (!is_operational()) {
            const std::vector<Operation> allowed_ops = {
                    Operation::ADD_HIDDEN,
                    Operation::RM_HIDDEN,
                    Operation::ADD_CONNECTION,
                    Operation::RM_CONNECTION,
                    Operation::STEP_WEIGHT,
                    Operation::STEP_BIAS,
                    Operation::RND_WEIGHT,
                    Operation::RND_BIAS,
            };

            while (!apply_operation(get_random_operation(allowed_ops))) {};
        }
        DEBUG("is operational");
    }

    // return random operation from the provided list, based on
    // related weights
    Operation get_random_operation(const std::vector<Operation>& ops)
    {
        assert(!ops.empty());

        std::vector<size_t> op_value(Operation::N_OPS, 0);
        size_t op_weights_sum = 0;
        for (auto& op : ops) {
            assert(op != Operation::N_OPS);
            assert(op < Operation::N_OPS);
            op_weights_sum += settings.op_weights[op];
            op_value[op] = op_weights_sum;
        }

        const size_t rnd_value = rododendrs::rnd01() * op_weights_sum;
        assert(Operation::ADD_INPUT == 0);
        for (size_t op = Operation::ADD_INPUT; op < Operation::N_OPS; op++) {
            if (op_value[op] == 0) {
                continue;
            }
            if (rnd_value < op_value[op]) {
                return (Operation)op;
            }
        }

        assert(false);
        return ops[0];
    }

    Operation get_random_operation()
    {
        std::vector<Operation> ops;
        assert(Operation::ADD_INPUT == 0);
        for (size_t op = 0; op < Operation::N_OPS; op++) {
            ops.push_back((Operation)op);
        }
        return get_random_operation(ops);
    }

    bool apply_operation(Operation op)
    {
        DEBUG("applying operation...");

        switch (op) {
            case Operation::ADD_INPUT:
                return _add_input().has_value();
            case Operation::RM_INPUT:
                if (_inputs_i.empty()) {
                    return false;
                }
                _rm_input(_inputs_i.rnd_i());
                return true;
            case Operation::ADD_OUTPUT:
                return _add_output().has_value();
            case Operation::RM_OUTPUT:
                if (_outputs_i.empty()) {
                    return false;
                }
                _rm_output(_outputs_i.rnd_i());
                return true;
            case Operation::ADD_HIDDEN:
                return _add_hidden().has_value();
            case Operation::RM_HIDDEN:
                if (_hidden_i.empty()) {
                    return false;
                }
                _rm_hidden(_hidden_i.rnd_i());
                return true;
            case Operation::ADD_CONNECTION:
                if (_g.n_vertices() < 2) {
                    return false;
                }
                return _add_connection(_g.rnd_vertex_i(), _g.rnd_vertex_i())
                        .has_value();
            case Operation::RM_CONNECTION:
                if (_g.n_edges() == 0) {
                    return false;
                }
                _rm_connection(_g.rnd_edge_i());
                return true;
            case Operation::STEP_WEIGHT:
                if (_g.n_edges() == 0) {
                    return false;
                }
                _step_weight(_g.rnd_edge_i());
                return true;
            case Operation::STEP_BIAS:
                if (_g.n_vertices() == 0) {
                    return false;
                }
                _step_bias(_g.rnd_vertex_i());
                return true;
            case Operation::RND_WEIGHT:
                if (_g.n_edges() == 0) {
                    return false;
                }
                _rnd_weight(_g.rnd_edge_i());
                return true;
            case Operation::RND_BIAS:
                if (_g.n_vertices() == 0) {
                    return false;
                }
                _rnd_bias(_g.rnd_vertex_i());
                return true;

            case Operation::N_OPS:
            default:
                // this should never happen
                assert(false);
                break;
        }

        // this should never happen
        assert(false);
        return false;
    }

    std::vector<double> infer(const std::vector<double> inputs)
    {
        DEBUG("infering...");

        // erase previous calculation result
        std::map<size_t, double> signals;
        std::set<size_t> calculated_i;

        // set input signals
        assert(inputs.size() == _inputs_i.size());
        for (size_t in_i = 0; in_i < _inputs_i.size(); in_i++) {
            const size_t vi = *_inputs_i.at(in_i);
            assert(!calculated_i.contains(vi));
            calculated_i.insert(vi);
            signals[vi] = inputs[in_i];
        }

        // calculate signal for every output
        std::vector<double> output_signals;
        for (size_t out_i = 0; out_i < _outputs_i.size(); out_i++) {
            const size_t vi = *_outputs_i.at(out_i);
            const double signal =
                    dfs_calculate_signal(vi, calculated_i, signals);
            output_signals.push_back(signal);
            signals[vi] = signal;
        }

        return output_signals;
    }

    // depth first search function that calculates signals of neurons
    double dfs_calculate_signal(size_t vertex_i,
                                std::set<size_t>& calculated_i,
                                std::map<size_t, double>& signals)
    {
        if (calculated_i.contains(vertex_i)) {
            return signals[vertex_i];
        }

        // - get Neuron = vi, bias
        // - sum = 0
        // - for every in connection
        //   - get weight
        //   - get ei
        //   - get src_vi
        //   - get signal(src_vi)
        //   - update sum
        // - apply acceptance_f(sum)
        const auto* v = _g.vertex_at(vertex_i);
        assert(v != nullptr);
        double sum                        = v->bias;
        const std::set<size_t> in_edges_i = v->_in_edges_i;
        for (size_t ei : in_edges_i) {
            const auto* e = _g.edge_at(ei);
            assert(e != nullptr);
            assert(e->_src_vertex_i.has_value());
            const size_t src_vi = e->_src_vertex_i.value();
            const double signal =
                    dfs_calculate_signal(src_vi, calculated_i, signals);
            signals[src_vi]     = signal;
            const double weight = e->weight;
            sum += weight * signal;
        }

        calculated_i.insert(vertex_i);
        return v->activation_f(sum);
    }

    void to_csv(const std::string& neurons_filepath,
                const std::string& connections_filepath)
    {
        _update_graphviz();
        _g.to_csv(neurons_filepath, connections_filepath);
    }

private:
    grafiins::DAG<Neuron, Connection> _g;
    garaza::Storage<size_t> _inputs_i;
    garaza::Storage<size_t> _outputs_i;
    garaza::Storage<size_t> _hidden_i;
    // connections are stored within _g

    std::optional<size_t> _add_input()
    {
        DEBUG("adding input...");

        assert(_inputs_i.size() <= settings.n_inputs);
        if (_inputs_i.size() == settings.n_inputs) {
            return {};
        }

        const size_t vi   = _g.add_vertex(Neuron(settings.neuron_afid));
        const size_t in_i = _inputs_i.add(vi);
        assert(_inputs_i.size() <= settings.n_inputs);
        return in_i;
    }

    void _rm_input(size_t i)
    {
        DEBUG("removing input...");

        assert(_inputs_i.contains_i(i));
        const size_t vi = *_inputs_i.at(i);
        assert(!_outputs_i.contains(vi));
        assert(!_hidden_i.contains(vi));
        assert(_g.contains_vertex_i(vi));
        // this will also update records in edges and adjucent vertices in _g
        _g.remove_vertex(vi);
        _inputs_i.remove(i);
    }

    std::optional<size_t> _add_output()
    {
        DEBUG("adding output...");

        assert(_outputs_i.size() <= settings.n_outputs);
        if (_outputs_i.size() == settings.n_outputs) {
            return {};
        }

        const size_t vi    = _g.add_vertex(Neuron(settings.neuron_afid));
        const size_t out_i = _outputs_i.add(vi);
        assert(_outputs_i.size() <= settings.n_outputs);
        return out_i;
    }

    void _rm_output(size_t i)
    {
        DEBUG("removing output...");

        assert(_outputs_i.contains_i(i));
        const size_t vi = *_outputs_i.at(i);
        assert(!_inputs_i.contains(vi));
        assert(!_hidden_i.contains(vi));
        assert(_g.contains_vertex_i(vi));
        // this will also update records in edges and adjucent vertices in _g
        _g.remove_vertex(vi);
        _outputs_i.remove(i);
    }

    std::optional<size_t> _add_hidden()
    {
        DEBUG("adding hidden...");

        assert(_hidden_i.size() <= settings.max_n_hidden);
        if (_hidden_i.size() == settings.max_n_hidden) {
            return {};
        }

        const size_t vi    = _g.add_vertex(Neuron(settings.neuron_afid));
        const size_t hid_i = _hidden_i.add(vi);
        assert(_hidden_i.size() <= settings.max_n_hidden);
        return hid_i;
    }

    void _rm_hidden(size_t i)
    {
        DEBUG("removing hidden...");

        assert(_hidden_i.contains_i(i));
        const size_t vi = *_hidden_i.at(i);
        assert(!_inputs_i.contains(vi));
        assert(!_outputs_i.contains(vi));
        assert(_g.contains_vertex_i(vi));
        // this will also update records in edges and adjucent vertices in _g
        _g.remove_vertex(vi);
        _hidden_i.remove(i);
    }

    // this function is needed, as the graph itself is not aware of
    // limitations on setting connections between inputs/outputs/hidden
    std::optional<size_t> _add_connection(size_t src_vi, size_t dst_vi)
    {
        DEBUG("adding connection...");

        if (_outputs_i.contains(src_vi) || _inputs_i.contains(dst_vi) ||
            dst_vi == src_vi) {
            return {};
        }

        // add edge
        const double init_weight = rododendrs::rnd_in_range(
                settings.min_init_weight, settings.max_init_weight);
        return _g.add_edge(Connection(src_vi, dst_vi, init_weight));
    }

    size_t _rm_connection(size_t ei)
    {
        DEBUG("removing connection...");

        assert(_g.contains_edge_i(ei));

        // this will also update records in adjucent vertices in _g
        return _g.remove_edge(ei);
    }

    void _step_weight(size_t ei)
    {
        DEBUG("stepping weight...");

        assert(_g.n_edges() > 0);
        auto* e = _g.edge_at(ei);
        assert(e != nullptr);
        const double weight_step = rododendrs::rnd_in_range(
                settings.min_weight_step, settings.max_weight_step);
        e->weight += weight_step;
        if (settings.limit_weight) {
            e->weight = std::min(e->weight, settings.max_weight);
            e->weight = std::max(e->weight, settings.min_weight);
        }
    }

    void _step_bias(size_t vi)
    {
        DEBUG("stepping bias...");

        assert(_g.n_vertices() > 0);
        auto* v = _g.vertex_at(vi);
        assert(v != nullptr);
        const double bias_step = rododendrs::rnd_in_range(
                settings.min_bias_step, settings.max_bias_step);
        v->bias += bias_step;
        if (settings.limit_bias) {
            v->bias = std::min(v->bias, settings.max_bias);
            v->bias = std::max(v->bias, settings.min_bias);
        }
    }

    void _rnd_weight(size_t ei)
    {
        DEBUG("randomizing weight...");

        assert(_g.n_edges() > 0);
        auto* e = _g.edge_at(ei);
        assert(e != nullptr);
        e->weight = rododendrs::rnd_in_range(settings.min_weight,
                                             settings.max_weight);
    }

    void _rnd_bias(size_t vi)
    {
        DEBUG("randomizing bias...");

        assert(_g.n_vertices() > 0);
        auto* v = _g.vertex_at(vi);
        assert(v != nullptr);
        v->bias =
                rododendrs::rnd_in_range(settings.min_bias, settings.max_bias);
    }

    void _update_graphviz()
    {
        for (size_t ii : _inputs_i.all_i()) {
            _update_graphviz_input(i, *_inputs_i.at(ii));
        }

        for (size_t oi : _outputs_i.all_i()) {
            _update_graphviz_output(i, *_outputs_i.at(oi));
        }

        for (size_t hi : _hidden_i.all_i()) {
            _update_graphviz_hidden(i, *_hidden_i.at(hi));
        }

        for (size_t ei : _g.all_edges_i()) {
            _update_graphviz_edge(ei);
        }
    }

    void _update_graphviz_input(size_t i, size_t vi)
    {
        auto* v = _g.vertex_at(vi);
        assert(v != nullptr);
        v->graphviz_shape   = settings.input_graphviz_shape;
        v->graphviz_cluster = settings.input_graphviz_cluster;
        v->graphviz_width   = settings.input_graphviz_width;
        v->label = "i" + std::to_string(i) + "(v" + std::to_string(vi) +
                   "):b=" + std::to_string(v->bias);
    }

    void _update_graphviz_output(size_t i, size_t vi)
    {
        auto* v = _g.vertex_at(vi);
        assert(v != nullptr);
        v->graphviz_shape   = settings.output_graphviz_shape;
        v->graphviz_cluster = settings.output_graphviz_cluster;
        v->graphviz_width   = settings.output_graphviz_width;
        v->label = "o" + std::to_string(i) + "(v" + std::to_string(vi) +
                   "):b=" + std::to_string(v->bias);
    }

    void _update_graphviz_hidden(size_t i, size_t vi)
    {
        auto* v = _g.vertex_at(vi);
        assert(v != nullptr);
        v->graphviz_shape   = settings.hidden_graphviz_shape;
        v->graphviz_cluster = settings.hidden_graphviz_cluster;
        v->graphviz_width   = settings.hidden_graphviz_width;
        v->label = "h" + std::to_string(i) + "(v" + std::to_string(vi) +
                   "):b=" + std::to_string(v->bias);
    }

    void _update_graphviz_edge(size_t ei)
    {
        auto* e = _g.edge_at(ei);
        assert(e != nullptr);
        e->label =
                "e" + std::to_string(ei) + ":w=" + std::to_string(e->weight);
    }
};

}  // namespace tante
