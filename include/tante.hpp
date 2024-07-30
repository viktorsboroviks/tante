#include <array>
#include <cassert>
#include <functional>
#include <map>
#include <queue>
#include <stdexcept>
#include <vector>

#include "grafiins.hpp"
#include "rododendrs.hpp"

#define DEBUG(x)                                  \
    do {                                          \
        std::cerr << "debug: " << x << std::endl; \
    } while (0)

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
    MV_CONNECTION_SRC,
    MV_CONNECTION_DST,
    STEP_WEIGHT,
    STEP_BIAS,
    N_OPS,
};

class Neuron : public grafiins::Vertex {
public:
    enum Type {
        NEURON_INPUT = 0,
        NEURON_OUTPUT,
        NEURON_HIDDEN,
        N_NTYPES,
    };

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

    Neuron(AFID afid, std::string label = "") :
        Vertex(label),
        activation_f(_get_af(afid))
    {
    }

    Neuron(std::string label = "") :
        Neuron(AF_RANDOM, label)
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
                throw std::runtime_error("Invalid AFID");
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
private:
    Settings _settings;
    grafiins::DAG<Neuron, Connection> _g;
    garaza::Storage<size_t> _inputs_i;
    garaza::Storage<size_t> _outputs_i;
    garaza::Storage<size_t> _hidden_i;
    // connections are stored within _g

    size_t _add_neuron(const Neuron::Type t,
                       const Neuron::AFID afid,
                       size_t i = garaza::I_RANDOM)
    {
        garaza::Storage& s;
        switch (t) {
            case Neuron::Type::NEURON_INPUT:
                DEBUG("adding input...");
                assert(i == garaza::I_FIRST_AVAILABLE ||
                       i == garaza::I_RANDOM || i < _settings.n_inputs);
                s = _inputs_i;
                break;
            case Neuron::Type::NEURON_OUTPUT:
                DEBUG("adding output...");
                assert(i == garaza::I_FIRST_AVAILABLE ||
                       i == garaza::I_RANDOM || i < _settings.n_outputs);
                s = _outputs_i;
                break;
            case Neuron::Type::NEURON_HIDDEN:
                DEBUG("adding hidden...");
                assert(i == garaza::I_FIRST_AVAILABLE ||
                       i == garaza::I_RANDOM || i < _settings.max_n_hidden);
                s = _hidden_i;
                break;
            default:
                throw std::runtime_error("Invalid neuron type.");
        }

        assert(!s.contains_i(i));
        const size_t vertex_i = _g.add_vertex(Neuron(afid));
        s.add(vertex_i, i);
        return i;
    }

    size_t _rm_neuron(const Neuron::Type t, size_t i = garaza::I_RANDOM)
    {
        garaza::Storage& s;
        switch (t) {
            case Neuron::Type::NEURON_INPUT:
                DEBUG("removing input...");
                assert(i == garaza::I_FIRST_AVAILABLE ||
                       i == garaza::I_RANDOM || i < _settings.n_inputs);
                s = _inputs_i;
                break;
            case Neuron::Type::NEURON_OUTPUT:
                DEBUG("removing output...");
                assert(i == garaza::I_FIRST_AVAILABLE ||
                       i == garaza::I_RANDOM || i < _settings.n_outputs);
                s = _outputs_i;
                break;
            case Neuron::Type::NEURON_HIDDEN:
                DEBUG("removing hidden...");
                assert(i == garaza::I_FIRST_AVAILABLE ||
                       i == garaza::I_RANDOM || i < _settings.max_n_hidden);
                s = _hidden_i;
                break;
            default:
                throw std::runtime_error("Invalid neuron type.");
        }

        assert(s.contains_i(i));
        // this will also update records in edges and adjucent vertices in _g
        const auto v* = _g.vertex_at(s[i]);
        assert(v != nullptr);
        _g.remove_vertex(s[i]);
        s.remove(i);
        return i;
    }

    // this function is needed, as the graph itself is not aware of
    // limitations on setting connections between inputs/outputs/hidden
    size_t _add_connection(size_t src_vi = garaza::I_RANDOM,
                           size_t dst_vi = garaza::I_RANDOM)
    {
        DEBUG("Adding connection...");
        if (src_vi == garaza::I_RANDOM) {
            src_vi = _g.rnd_vertex_i();
        }

        if (dst_vi == garaza::I_RANDOM) {
            dst_vi = _g.rnd_vertex_i();
        }

        if (_outputs_i.contains(src_vi) || _inputs_i.contains(dst_vi) ||
            dst_vi == src_vi) {
            throw std::logic_error("Invalid combination of src_vi, dst_vi.")
        }

        // add edge
        const double init_weight = rnd_in_range(_settings.min_init_weight,
                                                _settings.max_init_weight);
        return _g.add_edge(Connection(src_vi, dst_vi, init_weight));
    }

    size_t _rm_connection(size_t ei = garaza::I_RANDOM)
    {
        DEBUG("Removing connection...");
        // this will also update records in adjucent vertices in _g
        return _g.remove_edge(ei);
    }

    size_t _mv_connection_src(size_t ei = garaza::I_RANDOM,
                              size_t src_vi = garaza::I_RANDOM)
    {
        DEBUG("Moving connection src...");
        const auto* e = _g.edge_at(ei);
        assert(e != nullptr);
        const size_t dst_vi = e->dst_vertex_i;
        const size_t new_ei = _add_connection(src_vi, dst_vi);
        _rm_connection(ei);
        return new_ei;
    }

    size_t _mv_connection_dst(size_t ei = garaza::I_RANDOM,
                              size_t dst_vi = garaza::I_RANDOM)
    {
        DEBUG("Moving connection dst...");
        const auto* e = _g.edge_at(ei);
        assert(e != nullptr);
        const size_t src_vi = e->src_vertex_i;
        const size_t new_ei = _add_connection(src_vi, dst_vi);
        _rm_connection(ei);
        return new_ei;
    }

    void _step_weight(size_t ei = garaza::I_RANDOM)
    {
        DEBUG("Stepping weight...");
        if (_g.all_edges_i().empty()) {
            throw std::logic_error("No connections to step weight.")
        }

        if (ei == garaza::I_RANDOM) {
            ei = _g.rnd_edge_i();
        }

        const auto* e = _g.edge_at(ei);
        assert(e != nullptr);
        const double weight_step = rnd_in_range(_settings.min_weight_step,
                                                _settings.max_weight_step);
        e->weight += weight_step;
    }

    void _step_bias(size_t vi = garaza::I_RANDOM)
    {
        DEBUG("Stepping bias...");
        if (_g.all_vertices_i().empty()) {
            throw std::logic_error("No neurons to step bias.")
        }

        if (vi == garaza::I_RANDOM) {
            vi = _g.rnd_vertex_i();
        }

        const auto* v = _g.vertex_at(vi);
        assert(v != nullptr);
        const double bias_step =
                rnd_in_range(_settings.min_bias_step, _settings.max_bias_step);
        e->bias += bias_step;
    }

    // return random operation from the provided list, based on
    // related weights
    Operation _random_operation(const std::vectior<Operation>& ops)
    {
        assert(!ops.empty());

        std::vector<size_t> op_value;
        size_t op_weights_sum = 0;
        for (auto& op : ops) {
            assert(op != Operation::N_OPS);
            assert(op < Operation::N_OPS);
            op_weights_sum += op;
            op_value.push_back(op_weights_sum);
        }

        const int rnd_value = rododendrs::rnd01() * op_weights_sum;
        for (Operation op = 0; op < op_value.size(); op++) {
            if (rnd_value < op_value[op]) {
                return op;
            }
        }

        throw std::runtime_error("Failed to select random operation.");
    }

    Operation _random_operation()
    {
        std::vector<Operation> ops;
        for (Operation op = 0; op < Operation::N_OPS; op++) {
            ops.push_back(op);
        }
        return _select_rnd_operation(ops);
    }

    //    bool _is_operational()
    //    {
    //        assert(_inputs_i.size() == _settings.n_inputs);
    //        assert(_outputs_i.size() == _settings.n_outputs);
    //        assert(_neurons.size() <= _settings.max_n_neurons);
    //
    // #ifndef NDEBUG
    //        for (size_t i : _inputs_i) {
    //            assert(_g.get_vertex(i) != nullptr);
    //            assert(!_outputs_i.contains(i));
    //            bool found = false;
    //            for (auto n : _neurons) {
    //                if (n.graph_i == i) {
    //                    found = true;
    //                }
    //            }
    //            assert(!found);
    //        }
    //
    //        for (size_t i : _outputs_i) {
    //            assert(_g.get_vertex(i) != nullptr);
    //            assert(!_inputs_i.contains(i));
    //            bool found = false;
    //            for (auto n : _neurons) {
    //                if (n.graph_i == i) {
    //                    found = true;
    //                }
    //            }
    //            assert(!found);
    //        }
    //        for (auto &n : _neurons) {
    //            assert(_g.get_vertex(n.graph_i) != nullptr);
    //            assert(!_inputs_i.contains(n.graph_i));
    //            assert(!_outputs_i.contains(n.graph_i));
    //        }
    // #endif
    //
    //        // every input has a connection to at least one output
    //        for (size_t i : _inputs_i) {
    //            if (!_g.are_connected_any({i}, _outputs_i)) {
    //                return false;
    //            }
    //        }
    //
    //        // every output has a connection to at least one input
    //        for (size_t i : _outputs_i) {
    //            if (!_g.are_connected_any(_inputs_i, {i})) {
    //                return false;
    //            }
    //        }
    //
    //        return true;
    //    }
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

    // keep applying random operations until the network becomes operational
    void restore_randomly()
    {
        // add missing inputs
        for (size_t i = 0; i < _settings.n_inputs; i++) {
            if (_inputs_i.at(i) == nullptr) {
                _add_neuron(Neuron::Type::NEURON_INPUT, _settings.afid, i);
            }
        }
        assert(_inputs_i.size() == _settings.n_inputs);

        // add missing outputs
        for (size_t i = 0; i < _settings.n_outputs; i++) {
            if (_outputs_i.at(i) == nullptr) {
                _add_neuron(Neuron::Type::NEURON_OUTPUT, _settings.afid, i);
            }
        }
        assert(_outputs_i.size() == _settings.n_outputs);

        // add connections and hidden neurons until the network is restored
        while (!_is_operational()) {
            std::cout << "debug: not operational" << std::endl;
            const std::vector<Operation> allowed_ops =
            { Operation::ADD_HIDDEN,
              Operation::RM_HIDDEN,
              Operation::ADD_CONNECTION,
              Operation::RM_CONNECTION,
              Operation::MV_CONNECTION_SRC,
              Operation::MV_CONNECTION_DST,
              Operation::STEP_WEIGHT,
              Operation::STEP_BIAS,
            }

            while (!apply_operation(_random_operation()));
        }
    }

    bool apply_operation(Operation op)
    {
        try {
            switch (op) {
                case Operation::ADD_INPUT:
                    _add_neuron(Neuron::Type::NEURON_INPUT);
                    break;
                case Operation::RM_INPUT:
                    _rm_neuron(Neuron::Type::NEURON_INPUT);
                    break;
                case Operation::ADD_OUTPUT:
                    _add_neuron(Neuron::Type::NEURON_OUTPUT);
                    break;
                case Operation::RM_OUTPUT:
                    _rm_neuron(Neuron::Type::NEURON_OUTPUT);
                    break;
                case Operation::ADD_HIDDEN:
                    _add_neuron(Neuron::Type::NEURON_HIDDEN);
                    break;
                case Operation::RM_HIDDEN:
                    _rm_neuron(Neuron::Type::NEURON_HIDDEN);
                    break;
                case Operation::ADD_CONNECTION:
                    _add_connection();
                    break;
                case Operation::RM_CONNECTION:
                    _rm_connection();
                    break;
                case Operation::MV_CONNECTION_SRC:
                    _mv_connection_src();
                    break;
                case Operation::MV_CONNECTION_DST:
                    _mv_connection_dst();
                    break;
                case Operation::STEP_WEIGHT:
                    _step_weight();
                    break;
                case Operation::STEP_BIAS:
                    _step_bias();
                    break;

                case Operation::N_OPS:
                default:
                    // this should never happen
                    throw std::runtime_error("Invalid operation.")
            }
        }
        catch (std::logic_error& e) {
            return false;
        }

        return true;
    }

    //    std::vector<double> infer(std::vector<double> inputs)
    //    {
    //        std::cout << "debug: infer" << std::endl;
    //
    //        std::set<size_t> calculated_i;
    //        std::map<size_t, double> signals;
    //
    //        // set input signals
    //        assert(inputs.size() == _inputs_i.size());
    //        for (size_t i = 0; i < _inputs_i.size(); i++) {
    //            const size_t in_i = _get_input_i(i);
    //            assert(!calculated_i.contains(in_i));
    //            calculated_i.insert(in_i);
    //            signals[in_i] = inputs[i];
    //        }
    //
    //        // calculate signal for every output
    //        std::vector<double> outputs;
    //        for (size_t i = 0; i < _outputs_i.size(); i++) {
    //            outputs.push_back(dfs_calculate_signal(
    //                    _get_output_i(i), calculated_i, signals));
    //        }
    //
    //        std::cout << "debug: calculated_i" << std::endl;
    //        for (size_t i : calculated_i) {
    //            std::cout << i << std::endl;
    //        }
    //
    //        std::cout << "debug: signals" << std::endl;
    //        std::map<size_t, double>::iterator it = signals.begin();
    //        while (it != signals.end()) {
    //            std::cout << it->first << ": " << it->second << std::endl;
    //            it++;
    //        }
    //        return outputs;
    //    }
    //
    //    // depth first search function that calculates signals of neurons
    //    double dfs_calculate_signal(size_t vertex_i,
    //                                std::set<size_t> &calculated_i,
    //                                std::map<size_t, double> &signals)
    //    {
    //        std::cout << "debug: vertex_i=" << vertex_i << std::endl;
    //        if (calculated_i.contains(vertex_i)) {
    //            return signals[vertex_i];
    //        }
    //
    //        if (!_neuron_exists(vertex_i)) {
    //            // if no such neuron exist, it can only be output
    //            assert(_outputs_i.contains(vertex_i));
    //            const std::vector<size_t> in_vertices_i =
    //                    _g.get_in_vertices_i(vertex_i);
    //            // every output should only have 1 incomming vertex
    //            std::cout << "debug: in_vertices_i.size()=" <<
    //            in_vertices_i.size()
    //                      << std::endl;
    //            assert(in_vertices_i.size() == 1);
    //            const size_t in_vertex_i = in_vertices_i[0];
    //            std::cout << "debug: in_vertex_i=" << in_vertex_i <<
    //            std::endl; return dfs_calculate_signal(in_vertex_i,
    //            calculated_i, signals);
    //        }
    //
    //        const Neuron n = _get_neuron(vertex_i);
    //        double sum = n.bias;
    //        const auto *v = _g.get_vertex(vertex_i);
    //        assert(v != nullptr);
    //        const std::set<size_t> in_edges_i = v->get_in_edges_i();
    //        for (size_t in_edge_i : in_edges_i) {
    //            const double weight = _get_connection(in_edge_i).weight;
    //            const auto *e = _g.get_edge(in_edge_i);
    //            assert(e != nullptr);
    //            const size_t in_vertex_i = e->src_vertex_i;
    //            const double signal =
    //                    dfs_calculate_signal(in_vertex_i, calculated_i,
    //                    signals);
    //            signals[in_vertex_i] = signal;
    //            sum += weight * signal;
    //        }
    //
    //        calculated_i.insert(vertex_i);
    //        return n.activation_f(sum);
    //    }
};

}  // namespace tante
