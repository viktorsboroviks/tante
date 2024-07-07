namespace tante {

// Refs
// - bgl: find in/out nodes - https://stackoverflow.com/questions/48608876/find-the-child-nodes-in-a-graph-using-boost
// - bgl: find cyclic paths - https://stackoverflow.com/questions/23771885/boost-graph-that-doesnt-allow-circular-references
// - bgl: add weights to edges - https://stackoverflow.com/questions/24366642/how-do-i-change-the-edge-weight-in-a-graph-using-the-boost-graph-library
// - bgl: bundled properties - https://www.boost.org/doc/libs/1_69_0/libs/graph/doc/bundles.html

// TODO
// - describe neuron dag travesion for evaluation w cache
// - how to store/address history?

// classes
// - enum Perturbation
//
// - Settings
//   - size_t max_n_neurons
//   - std::array<size_t> perturbation_weights
//
// - Neuron
//   - enum activation_function
//   - double bias
//   - std::vector<double> signal_cache
//
// - Input
//
// - Output
//
// - Connection
//   - double weight
//
// - Net
//   - dag
//   - std::vector<double> get_output(input)
//   - void perturbate(rnd01)
//   - void randomize(rnd01)
//   - void _add_neuron(rnd01)

// Notes
// neuron
// - activation_function
// - bias
// - signal_cache
// - signal_cache_updated (find a better name)
//   - signal_cache_modified
// - inputs
//   - might become needed for marking data unused or removing orphan graphs
// - outputs
//   - must be recalculated if signal_cache_updated = true
// - inputs_allowed
// - outputs_allowed
//   - instead - make input/output a separate class
// - unused
//   - needed to be able to preserve history or grow paths that do not provide
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
// - perturbate (perturbation classes)
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
//   - make random perturbations until first signal is passed from every output
//   - a working net is needed to be able to perform its evaluations
//     after every perturbation later
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

}
