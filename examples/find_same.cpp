#include "lapsa.hpp"
#include "tante.hpp"

#define DEBUG(x)                                  \
    do {                                          \
        std::cerr << "debug: " << x << std::endl; \
    } while (0)

tante::Settings g_ts{};
const size_t g_n_inputs = 1;
const size_t g_n_outputs = 1;
const size_t g_max_n_hidden = 5;
const size_t g_max_op_weight = 1;
const double g_min_init_weight = -0.1;
const double g_max_init_weight = 0.1;
const double g_min_weight_step = -0.1;
const double g_max_weight_step = 0.1;
const double g_min_bias_step = -0.1;
const double g_max_bias_step = 0.1;

const size_t g_op_w_add_input = 1;
const size_t g_op_w_rm_input = 1;
const size_t g_op_w_add_output = 1;
const size_t g_op_w_rm_output = 1;
const size_t g_op_w_add_hidden = 1;
const size_t g_op_w_rm_hidden = 1;
const size_t g_op_w_add_connection = 1;
const size_t g_op_w_rm_connection = 1;
const size_t g_op_w_step_weight = 1;
const size_t g_op_w_step_bias = 1;

const size_t g_n_states = 1000000;
const size_t g_progress_update_period = 100;
const double g_init_p_acceptance = 0.97;
const size_t g_init_t_log_len = 100;
const double g_cooling_rate = (1 - 1e-4);
const size_t g_cooling_round_len = 1;

const std::string g_log_filename = "find_same_log.csv";

class MyState : public lapsa::State {
private:
    tante::Network _n;

public:
    MyState(lapsa::Settings &in_settings) :
        State(in_settings),
        _n{g_ts}
    {
    }

    double get_energy()
    {
        // if energy not calculated, do it now and store the result
        if (!_energy_calculated) {
            std::vector<double> inputs;
            const double training_data = rand() % 1000;
            inputs.push_back(training_data);
            assert(inputs.size() == g_n_inputs);
            const std::vector<double> outputs = _n.infer(inputs);
            assert(outputs.size() == g_n_outputs);
            const double result = outputs[0];
            _energy = std::abs(training_data - result);
            _energy_calculated = true;
        }
        return _energy;
    }

    void randomize()
    {
        _n.restore_randomly();
        reset_energy();
    }

    void change()
    {
        while (!_n.apply_operation(_n.get_random_operation()));
        _n.restore_randomly();
        reset_energy();
    }
};

void init_global_vars()
{
    g_ts.n_inputs = g_n_inputs;
    g_ts.n_outputs = g_n_outputs;
    g_ts.max_n_hidden = g_max_n_hidden;
    g_ts.max_op_weight = g_max_op_weight;
    g_ts.min_init_weight = g_min_init_weight;
    g_ts.max_init_weight = g_max_init_weight;
    g_ts.min_weight_step = g_min_weight_step;
    g_ts.max_weight_step = g_max_weight_step;
    g_ts.min_bias_step = g_min_bias_step;
    g_ts.max_bias_step = g_max_bias_step;

    g_ts.op_weights[tante::Operation::ADD_INPUT] = g_op_w_add_input;
    g_ts.op_weights[tante::Operation::RM_INPUT] = g_op_w_rm_input;
    g_ts.op_weights[tante::Operation::ADD_OUTPUT] = g_op_w_add_output;
    g_ts.op_weights[tante::Operation::RM_OUTPUT] = g_op_w_rm_output;
    g_ts.op_weights[tante::Operation::ADD_HIDDEN] = g_op_w_add_hidden;
    g_ts.op_weights[tante::Operation::RM_HIDDEN] = g_op_w_rm_hidden;
    g_ts.op_weights[tante::Operation::ADD_CONNECTION] = g_op_w_add_connection;
    g_ts.op_weights[tante::Operation::RM_CONNECTION] = g_op_w_rm_connection;
    g_ts.op_weights[tante::Operation::STEP_WEIGHT] = g_op_w_step_weight;
    g_ts.op_weights[tante::Operation::STEP_BIAS] = g_op_w_step_bias;
}

int main()
{
    init_global_vars();

    lapsa::Settings ls{};
    ls.n_states = g_n_states;
    ls.progress_update_period = g_progress_update_period;
    ls.init_p_acceptance = g_init_p_acceptance;
    ls.init_t_log_len = g_init_t_log_len;
    ls.cooling_rate = g_cooling_rate;
    ls.cooling_round_len = g_cooling_round_len;
    ls.log_filename = g_log_filename;

    lapsa::StateMachine<MyState> lsm{ls};
    lsm.init_functions = {
            lapsa::init_log<MyState>,
            lapsa::randomize_state<MyState>,
    };
    lsm.init_loop_functions = {
            lapsa::propose_new_state<MyState>,
            lapsa::record_init_temperature<MyState>,
            lapsa::select_init_temperature_as_max<MyState>,
            lapsa::init_run_progress<MyState>,
            lapsa::check_init_done<MyState>,
    };
    lsm.run_loop_functions = {
            lapsa::propose_new_state<MyState>,
            lapsa::decide_to_cool<MyState>,
            lapsa::cool_at_rate<MyState>,
            lapsa::update_state<MyState>,
            lapsa::check_run_done<MyState>,
            lapsa::update_log<MyState>,
            lapsa::print_run_progress<MyState>,
    };
    lsm.finalize_functions = {
            lapsa::clear_run_progress<MyState>,
            lapsa::print_stats<MyState>,
            lapsa::create_stats_file<MyState>,
    };
    lsm.run();
    return 0;
}
