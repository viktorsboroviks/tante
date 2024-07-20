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
    std::vector<size_t> _inputs_i;
    std::vector<size_t> _outputs_i;
    std::vector<size_t> _neurons_i;
    //    std::queue<size_t> _unallocated_neurons_i;

    void _add_neuron(const std::function<double(void)> &rnd01)
    {
        //        int vi = _g.add_vertex(grafiins::Vertex(std::to_string(i)));
        //        assert(vi >= 0);
        //        _inputs_i[i] = vi;
        std::cout << "debug: add neuron" << std::endl;
        // TODO: implement
        (void)rnd01;
    }

    void _rm_neuron(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: rm neuron" << std::endl;
        // TODO: implement
        (void)rnd01;
    }

    void _add_connection(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: add connection" << std::endl;
        // TODO: implement
        (void)rnd01;
    }

    void _rm_connection(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: rm connection" << std::endl;
        // TODO: implement
        (void)rnd01;
    }

    void _mv_connection(const std::function<double(void)> &rnd01)
    {
        std::cout << "debug: mv connection" << std::endl;
        // TODO: implement
        (void)rnd01;
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

        std::set<size_t> set_inputs_i{_inputs_i.begin(), _inputs_i.end()};
        std::set<size_t> set_outputs_i{_outputs_i.begin(), _outputs_i.end()};
        std::set<size_t> set_neurons_i{_neurons_i.begin(), _neurons_i.end()};
#ifndef NDEBUG
        assert(set_inputs_i.size() == _settings.n_inputs);
        assert(set_outputs_i.size() == _settings.n_outputs);
        assert(set_neurons_i.size() <= _settings.max_n_neurons);

        for (size_t i : set_inputs_i) {
            assert(_g.get_vertex(i) != nullptr);
            assert(!set_outputs_i.contains(i));
            assert(!set_neurons_i.contains(i));
        }
        for (size_t i : set_outputs_i) {
            assert(_g.get_vertex(i) != nullptr);
            assert(!set_inputs_i.contains(i));
            assert(!set_neurons_i.contains(i));
        }
        for (size_t i : set_neurons_i) {
            assert(_g.get_vertex(i) != nullptr);
            assert(!set_inputs_i.contains(i));
            assert(!set_outputs_i.contains(i));
        }
#endif

        // every input has a connection to at least one output
        for (size_t i : set_inputs_i) {
            if (!_g.are_connected_any({i}, set_outputs_i)) {
                return false;
            }
        }

        // every output has a connection to at least one input
        for (size_t i : set_outputs_i) {
            if (!_g.are_connected_any(set_inputs_i, {i})) {
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
        _inputs_i.resize(_settings.n_inputs);
        for (size_t i = 0; i < _settings.n_inputs; i++) {
            int vi = _g.add_vertex(grafiins::Vertex("in" + std::to_string(i)));
            assert(vi >= 0);
            _inputs_i[i] = vi;
        }

        // add output probes
        assert(_outputs_i.size() == 0);
        _outputs_i.resize(_settings.n_outputs);
        for (size_t i = 0; i < _settings.n_outputs; i++) {
            int vi =
                    _g.add_vertex(grafiins::Vertex("out" + std::to_string(i)));
            assert(vi >= 0);
            _outputs_i[i] = vi;
        }

        // restore network (necessary for energy calculation)
        _restore(rnd01);
    }

    virtual void apply_random_operation(
            const std::function<double(void)> &rnd01)
    {
        switch (_get_random_operation(rnd01)) {
            case Operation::ADD_NEURON:
                _add_neuron(rnd01);
                break;
            case Operation::RM_NEURON:
                _rm_neuron(rnd01);
                break;
            case Operation::ADD_CONNECTION:
                _add_connection(rnd01);
                break;
            case Operation::RM_CONNECTION:
                _rm_connection(rnd01);
                break;
            case Operation::MV_CONNECTION:
                _mv_connection(rnd01);
                break;
            default:
                // this should never happen
                assert(false);
                break;
        }
    }
};

}  // namespace tante
