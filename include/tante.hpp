#include <array>
#include <cassert>
#include <functional>
#include <queue>
#include <vector>

#include "grafiins.hpp"

namespace tante {

enum Operation {
    ADD_NEURON = 0,
    RM_NEURON,
    ADD_CONNECTION,
    RM_CONNECTION,
    MV_CONNECTION,
    N_OPS,
};

struct Settings {
    size_t n_inputs = 0;
    size_t n_outputs = 0;
    size_t max_n_neurons = 0;
    size_t max_op_weight = 100;
    size_t op_weights[Operation::N_OPS] = {0};
};

struct Neuron {
    enum ActivationF {
        SIGMOID = 0,
        N_AFS
    };

    ActivationF activation_f;
    double bias;
};

struct Connection {
    double weight;
};

class Network {
private:
    Settings _settings;
    grafiins::DAG<grafiins::Vertex, grafiins::Edge> _g;
    std::set<size_t> _inputs_i;
    std::set<size_t> _outputs_i;
    std::set<size_t> _neurons_i;

    bool _add_neuron(const std::function<double(void)> &rnd01)
    {
        (void)rnd01;
        std::cout << "debug: adding neuron..." << std::endl;

        assert(_settings.max_n_neurons > 0);
        if (_neurons_i.size() == _settings.max_n_neurons) {
            return false;
        }
        assert(_neurons_i.size() < _settings.max_n_neurons);

        const int vi = _g.add_vertex(grafiins::Vertex());
        assert(vi >= 0);
        auto *v = _g.get_vertex(vi);
        v->label = "n" + std::to_string(vi);
        _neurons_i.insert(vi);

        std::cout << "debug: added neuron " << v->label << std::endl;
        return true;
    }

    bool _rm_neuron(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: removing neuron..." << std::endl;
        if (_neurons_i.empty()) {
            return false;
        }

        const size_t vi_i = rnd01() * _neurons_i.size();
        std::set<size_t>::iterator it = _neurons_i.begin();
        std::advance(it, vi_i);
        const size_t vi = *it;
        assert(_neurons_i.contains(vi));
        assert(!_inputs_i.contains(vi));
        assert(!_outputs_i.contains(vi));

        const auto *v = _g.get_vertex(vi);
        assert(v != nullptr);
        std::cout << "debug: removed neuron " << v->label << std::endl;
        _g.remove_vertex(vi);
        _neurons_i.erase(vi);
        return true;
    }

    bool _add_connection(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: adding connection..." << std::endl;

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

        std::cout << "debug: added connection " << e->label << std::endl;
        return true;
    }

    bool _rm_connection(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: removing connection..." << std::endl;
        const auto all_ei = _g.get_edges_i();
        if (all_ei.empty()) {
            return false;
        }

        const size_t ei_i = rnd01() * all_ei.size();
        const size_t ei = all_ei[ei_i];

        const auto *e = _g.get_edge(ei);
        assert(e != nullptr);
        std::cout << "debug: removed connection " << e->label << std::endl;
        _g.remove_edge(ei);
        return true;
    }

    bool _mv_connection(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: mv connection" << std::endl;
        // TODO: implement
        (void)rnd01;
        return false;
    }

    size_t _get_random_operation(const std::function<double(void)> &rnd01)
    {
        std::vector<size_t> op_value;
        op_value.resize(Operation::N_OPS);
        size_t op_value_sum = 0;

        for (size_t i = 0; i < op_value.size(); i++) {
            op_value_sum += _settings.op_weights[i];
            op_value[i] = op_value_sum;
#ifndef NDEBUG
            switch (i) {
                case Operation::ADD_NEURON:
                case Operation::RM_NEURON:
                case Operation::ADD_CONNECTION:
                case Operation::RM_CONNECTION:
                case Operation::MV_CONNECTION:
                    break;
                default:
                    // this should never happen
                    assert(false);
                    break;
            }
#endif
        }

#ifndef NDEBUG
        size_t prev_value = 0;
        for (size_t v : op_value) {
            assert(v >= prev_value);
            prev_value = v;
        }
#endif
        const size_t rnd_value = rnd01() * op_value_sum;
        for (size_t i = 0; i < op_value.size(); i++) {
            if (rnd_value < op_value[i]) {
                return i;
            }
        }

        // this should never happen
        assert(false);
        return 0;
    }

    bool _is_operational()
    {
        assert(_inputs_i.size() == _settings.n_inputs);
        assert(_outputs_i.size() == _settings.n_outputs);
        assert(_neurons_i.size() <= _settings.max_n_neurons);

#ifndef NDEBUG
        for (size_t i : _inputs_i) {
            assert(_g.get_vertex(i) != nullptr);
            assert(!_outputs_i.contains(i));
            assert(!_neurons_i.contains(i));
        }
        for (size_t i : _outputs_i) {
            assert(_g.get_vertex(i) != nullptr);
            assert(!_inputs_i.contains(i));
            assert(!_neurons_i.contains(i));
        }
        for (size_t i : _neurons_i) {
            assert(_g.get_vertex(i) != nullptr);
            assert(!_inputs_i.contains(i));
            assert(!_outputs_i.contains(i));
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
                case Operation::ADD_NEURON:
                    op_applied = _add_neuron(rnd01);
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
                case Operation::MV_CONNECTION:
                    op_applied = _mv_connection(rnd01);
                    break;
                default:
                    // this should never happen
                    assert(false);
                    break;
            }
        } while (!op_applied);
    }
};

}  // namespace tante
