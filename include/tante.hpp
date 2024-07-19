#include <array>
#include <cassert>
#include <functional>
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

    void _add_neuron(const std::function<double(void)> &rnd01)
    {
        // TODO: implement
        (void)rnd01;
    }

    void _remove_neuron(const std::function<double(void)> &rnd01)
    {
        // TODO: implement
        (void)rnd01;
    }

    void _add_connection(const std::function<double(void)> &rnd01)
    {
        // TODO: implement
        (void)rnd01;
    }

    void _remove_connection(const std::function<double(void)> &rnd01)
    {
        // TODO: implement
        (void)rnd01;
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

        // every output has a connection to at least one input
        // TODO: add

        // every input has a connection to at least one input
        // TODO: add

        return true;
    }

    void _restore(const std::function<double(void)> &rnd01)
    {
        while (!_is_operational()) {
            // TODO: add
        }
        // TODO: implement
        (void)rnd01;
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
        // TODO: add

        // add output probes
        // TODO: add

        // restore network (necessary for energy calculation)
        // TODO: add
        (void)rnd01;
    }

    virtual void apply_operation(const std::function<double(void)> &rnd01)
    {
        // TODO: add
        (void)rnd01;
    }
};

}  // namespace tante
