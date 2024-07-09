#include "lapsa.hpp"
#include "tante.hpp"

tante::Settings g_ts{};
const size_t g_n_inputs = 1;
const size_t g_n_outputs = 1;
const size_t g_max_n_neurons = 5;
const size_t g_max_net_change_op_weight = 1;
const size_t g_w_add_neuron = 1;
const size_t g_w_remove_neuron = 1;
const size_t g_w_add_connection = 1;
const size_t g_w_remove_connection = 1;
const size_t g_w_move_connection = 1;

const size_t g_n_states = 1000000;
const size_t g_progress_update_period = 100;
const double g_init_p_acceptance = 0.97;
const size_t g_init_t_log_len = 100;
const double g_cooling_rate = (1 - 1e-4);
const size_t g_cooling_round_len = 1;

const std::string g_log_filename = "find_sin_log.csv";

class MyState : lapsa::State {
private:
    tante::Net _net;

public:
    MyState(lapsa::Settings &in_settings) :
        State(in_settings),
        _net{g_ts}
    {
    }

    double get_energy()
    {
        // TODO: add
        return 0;
    }

    void randomize(const std::function<double(void)> &rnd01)
    {
        // TODO: add
        (void)rnd01;
    }

    void change(const std::function<double(void)> &rnd01)
    {
        // TODO: add
        (void)rnd01;
    }
};

void init_global_vars()
{
    g_ts.n_inputs = g_n_inputs;
    g_ts.n_outputs = g_n_outputs;
    g_ts.max_n_neurons = g_max_n_neurons;
    g_ts.max_net_change_op_weight = g_max_net_change_op_weight;
    g_ts.net_change_op_weights[(size_t)tante::NetChangeOp::ADD_NEURON] =
            g_w_add_neuron;
    g_ts.net_change_op_weights[(size_t)tante::NetChangeOp::REMOVE_NEURON] =
            g_w_remove_neuron;
    g_ts.net_change_op_weights[(size_t)tante::NetChangeOp::ADD_CONNECTION] =
            g_w_add_connection;
    g_ts.net_change_op_weights[(size_t)tante::NetChangeOp::REMOVE_CONNECTION] =
            g_w_remove_connection;
    g_ts.net_change_op_weights[(size_t)tante::NetChangeOp::MOVE_CONNECTION] =
            g_w_move_connection;
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
            //            lapsa::randomize_state<MyState>,
    };
    //    sm.init_loop_functions = {
    //            lapsa::propose_new_state<MyState>,
    //            lapsa::record_init_temperature<MyState>,
    //            lapsa::select_init_temperature_as_max<MyState>,
    //            lapsa::init_run_progress<MyState>,
    //            lapsa::check_init_done<MyState>,
    //    };
    //    sm.run_loop_functions = {
    //            lapsa::propose_new_state<MyState>,
    //            lapsa::decide_to_cool<MyState>,
    //            lapsa::cool_at_rate<MyState>,
    //            lapsa::update_state<MyState>,
    //            lapsa::check_run_done<MyState>,
    //            lapsa::update_log<MyState>,
    //            lapsa::print_run_progress<MyState>,
    //    };
    //    sm.finalize_functions = {
    //            lapsa::clear_run_progress<MyState>,
    //            lapsa::print_stats<MyState>,
    //            lapsa::create_stats_file<MyState>,
    //    };
    lsm.run();
    return 0;
}
