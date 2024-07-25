#include <array>
#include <cassert>
#include <functional>
#include <queue>
#include <vector>

#include "grafiins.hpp"

namespace tante {

enum Operation {
    ADD_NEURON_SIGMOID = 0,
    ADD_NEURON_TANH,
    ADD_NEURON_RELU,
    RM_NEURON,
    ADD_CONNECTION,
    RM_CONNECTION,
    MV_CONNECTION_SRC,
    MV_CONNECTION_DST,
    STEP_WEIGHT,
    STEP_BIAS,
    N_OPS,
};

struct Settings {
    size_t n_inputs = 0;
    size_t n_outputs = 0;
    size_t max_n_neurons = 0;
    size_t max_op_weight = 100;
    size_t op_weights[Operation::N_OPS] = {0};
    size_t min_weight_step = 1;
    size_t max_weight_step = -1;
    size_t min_bias_step = 1;
    size_t max_bias_step = -1;
};

class Neuron {
public:
    const size_t graph_i;
    typedef std::function<double(double)> ActivationF;
    const ActivationF activation_f;
    double bias = 0;

    Neuron(size_t i, ActivationF af) :
        graph_i(i),
        activation_f(af)
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
};

inline bool operator<(const Neuron &lhs, const Neuron &rhs)
{
    return lhs.graph_i < rhs.graph_i;
}

class Connection {
public:
    size_t graph_i;
    double weight = 0;

    Connection(size_t i) :
        graph_i(i)
    {
    }
};

inline bool operator<(const Connection &lhs, const Connection &rhs)
{
    return lhs.graph_i < rhs.graph_i;
}

class Network {
private:
    Settings _settings;
    grafiins::DAG<grafiins::Vertex, grafiins::Edge> _g;
    std::set<size_t> _inputs_i;
    std::set<size_t> _outputs_i;
    std::set<Neuron> _neurons;
    std::set<Connection> _connections;

    const Neuron _get_neuron(size_t i)
    {
        assert(!_neurons.empty());

        for (auto &n : _neurons) {
            if (n.graph_i == i)
                return n;
        }

        // this should never happen
        assert(false);
    }

    const Connection _get_connection(size_t i)
    {
        assert(!_connections.empty());

        for (auto &c : _connections) {
            if (c.graph_i == i)
                return c;
        }

        // this should never happen
        assert(false);
    }

    size_t _get_input_i(size_t i_i)
    {
        assert(!_inputs_i.empty());
        assert(i < _inputs_i.size());
        std::set<size_t>::iterator it = _inputs_i.begin();
        std::advance(it, i_i);
        return (*it);
    }

    size_t _get_output_i(size_t i_i)
    {
        assert(!_outputs_i.empty());
        assert(i < _outputs_i.size());
        std::set<size_t>::iterator it = _outputs_i.begin();
        std::advance(it, i_i);
        return (*it);
    }

    const Neuron _get_rnd_neuron(const std::function<double(void)> &rnd01)
    {
        assert(!_neurons.empty());
        const size_t vi_i = rnd01() * _neurons.size();
        std::set<Neuron>::iterator it = _neurons.begin();
        std::advance(it, vi_i);
        return (*it);
    }

    const Connection _get_rnd_connection(
            const std::function<double(void)> &rnd01)
    {
        assert(!_connections.empty());
        const size_t ei_i = rnd01() * _connections.size();
        std::set<Connection>::iterator it = _connections.begin();
        std::advance(it, ei_i);
        return (*it);
    }

    bool _add_neuron(const std::function<double(void)> &rnd01,
                     Neuron::ActivationF af)
    {
        (void)rnd01;
        std::cout << "debug: adding neuron..." << std::endl;

        assert(_settings.max_n_neurons > 0);
        if (_neurons.size() == _settings.max_n_neurons) {
            return false;
        }
        assert(_neurons.size() < _settings.max_n_neurons);

        const int vi = _g.add_vertex(grafiins::Vertex());
        assert(vi >= 0);
        auto *v = _g.get_vertex(vi);
        v->label = "n" + std::to_string(vi);
        _neurons.insert(Neuron(vi, af));

        std::cout << "debug: added neuron " << v->label << std::endl;
        return true;
    }

    bool _rm_neuron(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: removing neuron..." << std::endl;
        if (_neurons.empty()) {
            return false;
        }

        const Neuron n = _get_rnd_neuron(rnd01);
        const int vi = n.graph_i;
        assert(!_inputs_i.contains(vi));
        assert(!_outputs_i.contains(vi));

        const auto *v = _g.get_vertex(vi);
        assert(v != nullptr);
        const std::string v_label = v->label;

        // vertex removal also removes all adjusted edges
        _g.remove_vertex(vi);
        for (Connection c : _connections) {
            if (_g.get_edge(c.graph_i) == nullptr) {
                _connections.erase(c);
            }
        }

#ifndef NDEBUG
        const size_t n_neurons = _neurons.size();
#endif
        _neurons.erase(n);
        assert(_neurons.size() == n_neurons - 1);

        std::cout << "debug: removed neuron " << v_label << std::endl;
        return true;
    }

    bool _add_connection(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: adding connection..." << std::endl;

        // TODO: fix here
        const auto all_vi = _g.get_vertices_i();
        size_t src_vi;
        size_t dst_vi;
        do {
            const size_t src_vi_i = rnd01() * all_vi.size();
            src_vi = all_vi[src_vi_i];
            const size_t dst_vi_i = rnd01() * all_vi.size();
            dst_vi = all_vi[dst_vi_i];
        } while (_outputs_i.contains(src_vi) || _inputs_i.contains(dst_vi) ||
                 dst_vi == src_vi);

        const int ei = _g.add_edge(grafiins::Edge(src_vi, dst_vi));
        if (ei < 0) {
            return false;
        }

        auto *v_src = _g.get_vertex(src_vi);
        auto *v_dst = _g.get_vertex(dst_vi);
        auto *e = _g.get_edge(ei);
        e->label = "e" + std::to_string(ei) + ":" + v_src->label + "->" +
                   v_dst->label;
        _connections.insert(Connection(ei));

        std::cout << "debug: added connection " << e->label << std::endl;
        return true;
    }

    bool _rm_connection(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: removing connection..." << std::endl;
        if (_connections.empty()) {
            return false;
        }

        const Connection c = _get_rnd_connection(rnd01);
        const size_t ei = c.graph_i;

        const auto *e = _g.get_edge(ei);
        assert(e != nullptr);
        const std::string e_label = e->label;

        _g.remove_edge(ei);
#ifndef NDEBUG
        size_t n_connections = _connections.size();
#endif
        _connections.erase(c);
        assert(_connections.size() == n_connections - 1);

        std::cout << "debug: removed connection " << e_label << std::endl;
        return true;
    }

    bool _mv_connection(const std::function<double(void)> &rnd01,
                        const bool move_src = false,
                        const bool move_dst = false)
    {
        assert(move_src || move_dst);

        std::cout << "debug: moving connection..." << std::endl;
        if (_connections.empty()) {
            return false;
        }

        const Connection c = _get_rnd_connection(rnd01);
        const size_t ei = c.graph_i;

        const auto all_vi = _g.get_vertices_i();
        auto e = *_g.get_edge(ei);
        size_t src_vi;
        if (move_src) {
            do {
                const size_t src_vi_i = rnd01() * all_vi.size();
                src_vi = all_vi[src_vi_i];
            } while (_outputs_i.contains(src_vi));

            if (src_vi == e.src_vertex_i) {
                return false;
            }
        }
        else {
            src_vi = e.src_vertex_i;
        }

        size_t dst_vi;
        if (move_dst) {
            do {
                const size_t dst_vi_i = rnd01() * all_vi.size();
                dst_vi = all_vi[dst_vi_i];
            } while (_outputs_i.contains(dst_vi));

            if (dst_vi == e.dst_vertex_i) {
                return false;
            }
        }
        else {
            dst_vi = e.dst_vertex_i;
        }

        if (src_vi == dst_vi) {
            return false;
        }

        e.src_vertex_i = src_vi;
        e.dst_vertex_i = dst_vi;
        const int new_ei = _g.add_edge(e);
        if (new_ei < 0) {
            return false;
        }

        auto *v_src = _g.get_vertex(e.src_vertex_i);
        auto *v_dst = _g.get_vertex(e.dst_vertex_i);
        e.label = "e" + std::to_string(new_ei) + ":" + v_src->label + "->" +
                  v_dst->label;

        _g.remove_edge(ei);
#ifndef NDEBUG
        size_t n_connections = _connections.size();
#endif
        Connection new_c = c;
        new_c.graph_i = new_ei;

        _connections.erase(c);
        assert(_connections.size() == n_connections - 1);

        _connections.insert(new_c);
        assert(_connections.size() == n_connections);

        std::cout << "debug: moved connection " << e.label << std::endl;
        return true;
    }

    bool _mv_connection_src(const std::function<double(void)> &rnd01)
    {
        return _mv_connection(rnd01, true, false);
    }

    bool _mv_connection_dst(const std::function<double(void)> &rnd01)
    {
        return _mv_connection(rnd01, false, true);
    }

    bool _step_weight(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: stepping weight..." << std::endl;
        if (_connections.empty()) {
            return false;
        }

        const Connection c_old = _get_rnd_connection(rnd01);
        const double weight_step = (rnd01() * (_settings.max_weight_step -
                                               _settings.min_weight_step)) +
                                   _settings.min_weight_step;
        assert(weight_step >= _settings.min_weight_step);
        assert(weight_step <= _settings.max_weight_step);

        Connection c_new = c_old;
        c_new.weight += weight_step;
        _connections.erase(c_old);
        _connections.insert(c_new);

        const std::string weight_step_sign = weight_step >= 0 ? "+" : "-";
        std::cout << "debug: stepped weight to " << c_new.weight << "("
                  << weight_step_sign << weight_step << ")" << std::endl;
        return true;
    }

    bool _step_bias(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: stepping bias..." << std::endl;
        if (_neurons.empty()) {
            return false;
        }

        const Neuron n_old = _get_rnd_neuron(rnd01);
        const double bias_step = (rnd01() * (_settings.max_bias_step -
                                             _settings.min_bias_step)) +
                                 _settings.min_bias_step;
        assert(bias_step >= _settings.min_bias_step);
        assert(bias_step <= _settings.max_bias_step);

        Neuron n_new = n_old;
        n_new.bias += bias_step;
        _neurons.erase(n_old);
        _neurons.insert(n_new);

        const std::string bias_step_sign = bias_step >= 0 ? "+" : "-";
        std::cout << "debug: stepped bias to " << n_new.bias << "("
                  << bias_step_sign << bias_step << ")" << std::endl;
        return true;
    }

    int _get_random_operation(const std::function<double(void)> &rnd01)
    {
        std::vector<int> op_value;
        op_value.resize(Operation::N_OPS);
        int op_value_sum = 0;

        for (size_t i = 0; i < op_value.size(); i++) {
            op_value_sum += _settings.op_weights[i];
            op_value[i] = op_value_sum;
#ifndef NDEBUG
            switch (i) {
                case Operation::ADD_NEURON_SIGMOID:
                case Operation::ADD_NEURON_TANH:
                case Operation::ADD_NEURON_RELU:
                case Operation::RM_NEURON:
                case Operation::ADD_CONNECTION:
                case Operation::RM_CONNECTION:
                case Operation::MV_CONNECTION_SRC:
                case Operation::MV_CONNECTION_DST:
                case Operation::STEP_WEIGHT:
                case Operation::STEP_BIAS:
                    break;
                default:
                    // this should never happen
                    assert(false);
                    break;
            }
#endif
        }

#ifndef NDEBUG
        int prev_value = 0;
        for (int v : op_value) {
            assert(v >= prev_value);
            prev_value = v;
        }
#endif
        const int rnd_value = rnd01() * op_value_sum;
        for (size_t i = 0; i < op_value.size(); i++) {
            if (rnd_value < op_value[i]) {
                return i;
            }
        }

        // this should never happen
        assert(false);
        return -1;
    }

    bool _is_operational()
    {
        assert(_inputs_i.size() == _settings.n_inputs);
        assert(_outputs_i.size() == _settings.n_outputs);
        assert(_neurons.size() <= _settings.max_n_neurons);

#ifndef NDEBUG
        for (size_t i : _inputs_i) {
            assert(_g.get_vertex(i) != nullptr);
            assert(!_outputs_i.contains(i));
            bool found = false;
            for (auto n : _neurons) {
                if (n.graph_i == i) {
                    found = true;
                }
            }
            assert(!found);
        }

        for (size_t i : _outputs_i) {
            assert(_g.get_vertex(i) != nullptr);
            assert(!_inputs_i.contains(i));
            bool found = false;
            for (auto n : _neurons) {
                if (n.graph_i == i) {
                    found = true;
                }
            }
            assert(!found);
        }
        for (auto &n : _neurons) {
            assert(_g.get_vertex(n.graph_i) != nullptr);
            assert(!_inputs_i.contains(n.graph_i));
            assert(!_outputs_i.contains(n.graph_i));
        }
#endif

        // every input has a connection to at least one output
        for (size_t i : _inputs_i) {
            if (!_g.are_connected_any({i}, _outputs_i)) {
                return false;
            }
        }

        // every output has a connection to at least one input
        for (size_t i : _outputs_i) {
            if (!_g.are_connected_any(_inputs_i, {i})) {
                return false;
            }
        }

        return true;
    }

    void _restore(const std::function<double(void)> &rnd01)
    {
        while (!_is_operational()) {
            std::cout << "debug: not operational" << std::endl;
            apply_random_operation(rnd01);
        }
    }

public:
    Network(Settings &in_settings) :
        _settings(in_settings)
    {
        assert(_settings.n_inputs > 0);
        assert(_settings.n_outputs > 0);
        assert(_settings.max_n_neurons > 0);
        assert(_settings.max_op_weight > 0);
#ifndef NDEBUG
        for (auto w : _settings.op_weights) {
            assert(w <= _settings.max_op_weight);
        }
#endif
    }

    void randomize(const std::function<double(void)> &rnd01)
    {
        // add input probes
        assert(_inputs_i.size() == 0);
        for (size_t i = 0; i < _settings.n_inputs; i++) {
            const int vi = _g.add_vertex(grafiins::Vertex());
            assert(vi >= 0);
            auto *v = _g.get_vertex(vi);
            v->label = "in" + std::to_string(vi);
            _inputs_i.insert(vi);
        }
        assert(_inputs_i.size() == _settings.n_inputs);

        // add output probes
        assert(_outputs_i.size() == 0);
        for (size_t i = 0; i < _settings.n_outputs; i++) {
            const int vi = _g.add_vertex(grafiins::Vertex());
            assert(vi >= 0);
            auto *v = _g.get_vertex(vi);
            v->label = "out" + std::to_string(vi);
            _outputs_i.insert(vi);
        }
        assert(_outputs_i.size() == _settings.n_outputs);

        // restore network (necessary for energy calculation)
        _restore(rnd01);
    }

    virtual void apply_random_operation(
            const std::function<double(void)> &rnd01)
    {
        bool op_applied = false;
        do {
            switch (_get_random_operation(rnd01)) {
                case Operation::ADD_NEURON_SIGMOID:
                    std::cout << "debug: adding neuron sigmoid..."
                              << std::endl;
                    op_applied = _add_neuron(rnd01, Neuron::af_sigmoid);
                    break;
                case Operation::ADD_NEURON_TANH:
                    std::cout << "debug: adding neuron tanh..." << std::endl;
                    op_applied = _add_neuron(rnd01, Neuron::af_tanh);
                    break;
                case Operation::ADD_NEURON_RELU:
                    std::cout << "debug: adding neuron relu..." << std::endl;
                    op_applied = _add_neuron(rnd01, Neuron::af_relu);
                    break;
                case Operation::RM_NEURON:
                    op_applied = _rm_neuron(rnd01);
                    break;
                case Operation::ADD_CONNECTION:
                    op_applied = _add_connection(rnd01);
                    break;
                case Operation::RM_CONNECTION:
                    op_applied = _rm_connection(rnd01);
                    break;
                case Operation::MV_CONNECTION_SRC:
                    op_applied = _mv_connection_src(rnd01);
                    break;
                case Operation::MV_CONNECTION_DST:
                    op_applied = _mv_connection_dst(rnd01);
                    break;
                case Operation::STEP_WEIGHT:
                    op_applied = _step_weight(rnd01);
                    break;
                case Operation::STEP_BIAS:
                    op_applied = _step_bias(rnd01);
                    break;
                default:
                    // this should never happen
                    assert(false);
                    break;
            }
        } while (!op_applied);
    }

    std::vector<double> infer(std::vector<double> inputs)
    {
        std::set<size_t> calculated_i;
        std::vector<double> signals;

        signals.resize(_g.get_n_vertices());

        // set input signals
        assert(inputs.size() == _inputs_i.size());
        for (size_t i = 0; i < _inputs_i.size(); i++) {
            const size_t in_i = _get_input_i(i);
            assert(!calculated_i.contains(in_i));
            calculated_i.insert(in_i);
            assert(in_i < signals.size());
            signals[in_i] = inputs[i];
        }

        std::vector<double> outputs;
        for (size_t i = 0; i < _outputs_i.size(); i++) {
            outputs.push_back(dfs_calculate_signal(i, calculated_i, signals));
        }

        return outputs;
    }

    double dfs_calculate_signal(size_t vertex_i,
                                std::set<size_t> &calculated_i,
                                std::vector<double> &signals)
    {
        // TODO: add
        //     - get in_vertices:
        //       - for every vertice -> run dfs_calculate_signal
        //     - run calculation
        //     - calculated.insert(v_i)

        assert(vertex_i < signals.size());
        if (calculated_i.contains(vertex_i)) {
            return signals[vertex_i];
        }

        const Neuron n = get_neuron(vertex_i);
        double sum = n.bias;
        const std::vector<size_t> in_edges_i = _g.get_in_edges_i(vertex_i);
        for (size_t in_edge_i : in_edges_i) {
            const double weight = get_connection(in_edge_i).weight;
            const size_t in_vertex_i = _g.get_edge(in_edge_i).src_vertex_i;
            const double signal =
                    dfs_calculate_signal(in_vertex_i, calculated_i, signals);
            signals[in_vertex_i] = signal;
            sum += weight * signal;
        }

        calculated_i.insert(vertex_i);
        return n.activation_f(sum);
    }
};

}  // namespace tante
