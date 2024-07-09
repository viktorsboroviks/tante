#include <array>
#include <boost/graph/adjacency_list.hpp>
#include <functional>
#include <vector>

namespace tante {

// Refs
// - bgl: find in/out nodes -
// https://stackoverflow.com/questions/48608876/find-the-child-nodes-in-a-graph-using-boost
// - bgl: find cyclic paths -
// https://stackoverflow.com/questions/23771885/boost-graph-that-doesnt-allow-circular-references
// - bgl: add weights to edges -
// https://stackoverflow.com/questions/24366642/how-do-i-change-the-edge-weight-in-a-graph-using-the-boost-graph-library
// - bgl: bundled properties -
// https://www.boost.org/doc/libs/1_69_0/libs/graph/doc/bundles.html

enum class NetChangeOp {
    ADD_NEURON = 0,
    REMOVE_NEURON,
    ADD_CONNECTION,
    REMOVE_CONNECTION,
    MOVE_CONNECTION,
    NUMBER_OF_NCOS,
};

struct Settings {
    size_t n_inputs = 0;
    size_t n_outputs = 0;
    size_t max_n_neurons = 0;
    size_t max_net_change_op_weight = 100;

    // address using NetChangeOp:
    // net_change_op_weights[NetChangeOp::ADD_NEURON]
    size_t net_change_op_weights[(size_t)NetChangeOp::NUMBER_OF_NCOS] = {0};
};

struct Node {
    enum class Type {
        NEURON = 0,
        INPUT_PROBE,
        OUTPUT_PROBE,
        NUMBER_OF_NTS
    };

    Node(Type in_type) :
        type(in_type)
    {
        assert(in_type >= Type::NEURON);
        assert(in_type < Type::NUMBER_OF_NTS);
    }

    Type type;

    bool is_neuron()
    {
        return type == Type::NEURON;
    }

    bool is_input_probe()
    {
        return type == Type::INPUT_PROBE;
    }

    bool is_output_probe()
    {
        return type == Type::OUTPUT_PROBE;
    }

    double output_signal;
    bool output_signal_calculated = false;  // temporary marker during
                                            // net output calculation

    enum class NeuronActivationFunction {
        SIGMOID = 0,
        // TODO: add more as needed
        NUMBER_OF_NAFS
    };

    NeuronActivationFunction neuron_activation_function;
    double neuron_bias;
};

struct Connection {
    double weight;
};

class Net {
private:
    // ref: https://www.boost.org/doc/libs/1_69_0/libs/graph/doc/bundles.html
    typedef boost::adjacency_list<boost::listS,
                                  boost::vecS,
                                  boost::directedS,
                                  Node,
                                  Connection>
            Graph;
    typedef Graph::vertex_descriptor Vertex;
    Settings _settings;
    Graph _g;

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

    bool _net_is_operational()
    {
        std::vector<size_t> input_probes_idx;
        std::vector<size_t> output_probes_idx;
        for (size_t i = 0; i < boost::num_vertices(_g); i++) {
            if (_g[i].is_input_probe()) {
                input_probes_idx.push_back(i);
            }
            else if (_g[i].is_output_probe()) {
                output_probes_idx.push_back(i);
            }
        }
        assert(input_probes_idx.size() == _settings.n_inputs);
        assert(output_probes_idx.size() == _settings.n_outputs);
        // every output has a connection to at least one input
        // every input has a connection to at least one input
        // TODO: add
        return true;
    }

    void _restore_net(const std::function<double(void)> &rnd01)
    {
        while (!_net_is_operational()) {
            //
        }
        // TODO: implement
        (void)rnd01;
    }

public:
    Net(Settings &in_settings) :
        _settings(in_settings)
    {
        assert(_settings.n_inputs > 0);
        assert(_settings.n_outputs > 0);
        assert(_settings.max_n_neurons > 0);
        assert(_settings.max_net_change_op_weight > 0);
#ifndef NDEBUG
        for (auto w : _settings.net_change_op_weights) {
            assert(w <= _settings.max_net_change_op_weight);
        }
#endif
    }

    void randomize(const std::function<double(void)> &rnd01)
    {
        // add input probes
        assert(boost::num_vertices(_g) == 0);
        for (size_t i = 0; i < _settings.n_inputs; i++) {
            Node n{Node::Type::INPUT_PROBE};
            boost::add_vertex(n, _g);
        }
        assert(boost::num_vertices(_g) == _settings.n_inputs);

        // add output probes
        for (size_t i = 0; i < _settings.n_outputs; i++) {
            Node n{Node::Type::OUTPUT_PROBE};
            boost::add_vertex(n, _g);
        }
        assert(boost::num_vertices(_g) ==
               _settings.n_inputs + _settings.n_outputs);

        // restore network (necessary for energy calculation)
        // TODO: implement
        (void)rnd01;
    }

    std::vector<double> get_outputs(std::vector<double> &inputs)
    {
        // TODO: implement
        (void)inputs;
        return std::vector<double>{0};
    }

    virtual void change(const std::function<double(void)> &rnd01)
    {
        // TODO: implement
        (void)rnd01;
    }
};

// NOTES:
// neuron
// - activation_function
// - bias
// - signal_cache
// - signal_cache_updated (find a better name)
//   - signal_cache_modified
// - inputs
//   - might become needed for marking data unused or removing orphan
//   graphs
// - outputs
//   - must be recalculated if signal_cache_updated = true
// - inputs_allowed
// - outputs_allowed
//   - instead - make input/output a separate class
// - unused
//   - needed to be able to preserve history or grow paths that do not
//   provide
//     value before finished
//   - mark unused
//     - if no outputs
//     - or if no inputs
//     - or if all outputs are unused
//     - or if no inputs
//     - or if all inputs are unused
//     - and still has at least 1 path connecting to the main graph
//   - do not recalculate
//   - return signal 0
//   - connections not impacted

// connection
// - in_neuron
// - out_neuron
// - weight

// network
// - inputs
// - outputs
// - dag
// - change
//   - add neuron
//   - remove neuron
//   - remove unused neuron
//   - change neuron activation function
//   - change neuron activation function parameters (bias)
//   - add connection
//   - remove connection
//   - change connection weight
// - limitations
// - run

// topology
// - dag
// - no cyclic paths
// - no parallel paths of len 1
// - no separate graphs
// - every node except in/out
//   - >=1 in
//   - >=1 out
// - in/out
//   - in
//     - only signal, no acceptance_function
//   - out
//     - only 1 input allowed (no sum of inputs)
//     - no acceptance function

// algorithms
// - init new net
//   - make random changes until first signal is passed from every output
//   - a working net is needed to be able to perform its evaluations
//     after every change later
// - restore net operability
//   - not implemented
//   - overcomplication for the first iteration
// - add new node
//   - propose graph with a new node
//   - check no cycles
//   - accept or repeat
// - remove node
//   - remove all related connections
// - remove unused node
//   - not implemented
//   - overcomplication for the first iteration
// - add new connection
// - remove connection
// - marking unused
//   - not implemented
//   - overcomplication for the first iteration
// - evaluating net
//   - calculate dependency tree from output
//   - traverse dependency tree
//   - save cache for every node
// - saving cache
//   - after calculation save result
// - updating signal cache
//   - on adding node
//     - mark new node as modified
//     - evaluate net
//       - calculdate dependency tree
//       - mark all out nodes of modified nodes as modified recursively
//       - traverse dependency tree and calculate the result
//   - on removing node
//     - mark out nodes of the removed node as "modified"
//     - mark all out nodes of modified nodes as modified recursively
//     - evaluate net
//   - on modifying node
//     - mark node as "modified"
//     - mark all out nodes of modified nodes as modified recursively
//     - evaluate net
//   - on adding connection
//     - mark out node as "modified"
//     - mark all out nodes of modified nodes as modified recursively
//     - evaluate net
//   - on removing connection
//     - mark out node as "modified"
//     - mark all out nodes of modified nodes as modified recursively
//     - evaluate net
//   - on modifying connection
//     - mark out node as "modified"
//     - mark all out nodes of modified nodes as modified recursively
//     - evaluate net

}  // namespace tante
