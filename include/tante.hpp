#include <array>
#include <cassert>
#include <functional>
#include <map>
#include <optional>
#include <queue>
#include <vector>

#define PRINT_DEBUGS
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
    N_OPS,
};

class Neuron : public grafiins::Vertex {
public:
    typedef std::function<double(double)> ActivationF;
    enum AFID {
        AF_RANDOM = -1,
        AF_TANH = 0,
        AF_SIGMOID,
        AF_RELU,
        N_AFS,
    };

    ActivationF activation_f;
    double bias = 0;

    Neuron(AFID afid = AF_TANH, std::string label = "") :
        Vertex(label),
        activation_f(_get_af(afid))
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

private:
    ActivationF _get_af(AFID afid)
    {
        const int rnd_afid = rododendrs::rnd01() * (double)N_AFS;
        ActivationF af;
        switch (afid) {
            case AF_RANDOM:
                af = _get_af((AFID)rnd_afid);
                break;
            case AF_TANH:
                af = af_tanh;
                break;
            case AF_SIGMOID:
                af = af_sigmoid;
                break;
            case AF_RELU:
                af = af_relu;
                break;
            case N_AFS:
            default:
                assert(false);
                break;
        }

        return af;
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
};

struct Settings {
    Neuron::AFID neuron_afid = Neuron::AFID::AF_SIGMOID;
    size_t n_inputs = 0;
    size_t n_outputs = 0;
    size_t max_n_hidden = 0;
    size_t max_op_weight = 100;
    size_t op_weights[Operation::N_OPS] = {0};
    double min_init_weight = -1;
    double max_init_weight = 1;
    double min_weight_step = 1;
    double max_weight_step = -1;
    double min_bias_step = 1;
    double max_bias_step = -1;
};

double rnd_in_range(double min, double max)
{
    if (min == max) {
        return min;
    }

    assert(min < max);
    const double retval = (rododendrs::rnd01() * (max - min)) + min;
    assert(retval >= min);
    assert(retval <= max);
    return retval;
}

class Network {
public:
    Network(Settings& in_settings) :
        _settings(in_settings)
    {
        assert(_settings.n_inputs > 0);
        assert(_settings.n_outputs > 0);
        assert(_settings.max_n_hidden > 0);
        assert(_settings.max_op_weight > 0);
#ifndef NDEBUG
        for (auto w : _settings.op_weights) {
            assert(w <= _settings.max_op_weight);
        }
#endif
        assert(_settings.min_init_weight <= _settings.max_init_weight);
        assert(_settings.min_weight_step <= _settings.max_weight_step);
        assert(_settings.min_bias_step <= _settings.max_bias_step);
    }

    bool is_operational()
    {
        DEBUG("checking if operational...");

        assert(_inputs_i.size() <= _settings.n_inputs);
        assert(_outputs_i.size() <= _settings.n_outputs);
        assert(_hidden_i.size() <= _settings.max_n_hidden);

        if (_inputs_i.empty()) {
            DEBUG("no inputs.");
            return false;
        }

        if (_outputs_i.empty()) {
            DEBUG("no outputs.");
            return false;
        }

        const std::list<size_t> list_inputs_vi = _inputs_i.list();
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
        const size_t add_n_inputs = _settings.n_inputs - _inputs_i.size();
        for (size_t i = 0; i < add_n_inputs; i++) {
            _add_input();
        }
        assert(_inputs_i.size() == _settings.n_inputs);

        // add missing outputs
        const size_t add_n_outputs = _settings.n_outputs - _outputs_i.size();
        for (size_t i = 0; i < add_n_outputs; i++) {
            _add_output();
        }
        assert(_outputs_i.size() == _settings.n_outputs);

        std::cout << "debug: inputs and outputs added" << std::endl;
        // add connections and hidden neurons until the network is restored
        while (!is_operational()) {
            const std::vector<Operation> allowed_ops = {
                    Operation::ADD_HIDDEN,
                    Operation::RM_HIDDEN,
                    Operation::ADD_CONNECTION,
                    //                    Operation::RM_CONNECTION,
                    Operation::STEP_WEIGHT,
                    Operation::STEP_BIAS,
            };

            size_t di = 0;
            while (!apply_operation(get_random_operation(allowed_ops))) {
                // debug
                di++;
                if (di > 5)
                    exit(-1);
            };
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
            op_weights_sum += _settings.op_weights[op];
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

        std::set<size_t> calculated_i;
        std::map<size_t, double> signals;

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
            output_signals.push_back(dfs_calculate_signal(
                    *_outputs_i.at(out_i), calculated_i, signals));
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
        double sum = v->bias;
        const std::set<size_t> in_edges_i = v->_in_edges_i;
        for (size_t ei : in_edges_i) {
            const auto* e = _g.edge_at(ei);
            assert(e != nullptr);
            assert(e->_src_vertex_i.has_value());
            const size_t src_vi = e->_src_vertex_i.value();
            const double signal =
                    dfs_calculate_signal(src_vi, calculated_i, signals);
            signals[src_vi] = signal;
            const double weight = e->weight;
            sum += weight * signal;
        }

        calculated_i.insert(vertex_i);
        return v->activation_f(sum);
    }

private:
    Settings _settings;
    grafiins::DAG<Neuron, Connection> _g;
    garaza::Storage<size_t> _inputs_i;
    garaza::Storage<size_t> _outputs_i;
    garaza::Storage<size_t> _hidden_i;
    // connections are stored within _g

    std::optional<size_t> _add_input()
    {
        DEBUG("adding input...");

        assert(_inputs_i.size() <= _settings.n_inputs);
        if (_inputs_i.size() == _settings.n_inputs) {
            return {};
        }

        const size_t vi = _g.add_vertex(Neuron(_settings.neuron_afid));
        const size_t in_i = _inputs_i.add(vi);
        assert(_inputs_i.size() <= _settings.n_inputs);
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

        assert(_outputs_i.size() <= _settings.n_outputs);
        if (_outputs_i.size() == _settings.n_outputs) {
            return {};
        }

        const size_t vi = _g.add_vertex(Neuron(_settings.neuron_afid));
        const size_t out_i = _outputs_i.add(vi);
        assert(_outputs_i.size() <= _settings.n_outputs);
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

        assert(_hidden_i.size() <= _settings.max_n_hidden);
        if (_hidden_i.size() == _settings.max_n_hidden) {
            return {};
        }

        const size_t vi = _g.add_vertex(Neuron(_settings.neuron_afid));
        const size_t hid_i = _hidden_i.add(vi);
        assert(_hidden_i.size() <= _settings.max_n_hidden);
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
        const double init_weight = rnd_in_range(_settings.min_init_weight,
                                                _settings.max_init_weight);
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
        const double weight_step = rnd_in_range(_settings.min_weight_step,
                                                _settings.max_weight_step);
        e->weight += weight_step;
    }

    void _step_bias(size_t vi)
    {
        DEBUG("stepping bias...");

        assert(_g.n_vertices() > 0);
        auto* v = _g.vertex_at(vi);
        assert(v != nullptr);
        const double bias_step =
                rnd_in_range(_settings.min_bias_step, _settings.max_bias_step);
        v->bias += bias_step;
    }
};

}  // namespace tante
