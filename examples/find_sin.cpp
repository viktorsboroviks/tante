#include "lapsa.hpp"
#include "tante.hpp"

const size_t g_n_inputs = 1;
const size_t g_n_outputs = 1;
const size_t g_max_n_neurons = 5;

const size_t g_n_states = 1000000;
const size_t g_progress_update_period = 100;
const double g_init_p_acceptance = 0.97;
const size_t g_init_t_log_len = 100;
const double g_cooling_rate = (1 - 1e-4);
const size_t g_cooling_round_len = 1;

const std::string g_log_filename = "find_sin_log.csv";

class MyState : lapsa::State {
public:
    MyState(lapsa::Settings &in_settings) :
        State(in_settings)
    {
        // TODO: add
        sin(a, b);
    }

    double get_energy()
    {
        // TODO: add
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

int main()
{
    lapsa::Settings s{};
    s.n_states = g_n_states;
    s.progress_update_period = g_progress_update_period;
    s.init_p_acceptance = g_init_p_acceptance;
    s.init_t_log_len = g_init_t_log_len;
    s.cooling_rate = g_cooling_rate;
    s.cooling_round_len = g_cooling_round_len;
    s.log_filename = g_log_filename;

    lapsa::StateMachine<MyState> sm{s};
    sm.init_functions = {
            lapsa::init_log<MyState>,
            lapsa::init_state<MyState>,
    };
    sm.init_loop_functions = {
            lapsa::propose_new_state<MyState>,
            lapsa::record_init_temperature<MyState>,
            lapsa::select_init_temperature_as_max<MyState>,
            lapsa::init_run_progress<MyState>,
            lapsa::check_init_done<MyState>,
    };
    sm.run_loop_functions = {
            lapsa::propose_new_state<MyState>,
            lapsa::decide_to_cool<MyState>,
            lapsa::cool_at_rate<MyState>,
            lapsa::update_state<MyState>,
            lapsa::check_run_done<MyState>,
            lapsa::update_log<MyState>,
            lapsa::print_run_progress<MyState>,
    };
    sm.finalize_functions = {
            lapsa::clear_run_progress<MyState>,
            lapsa::print_stats<MyState>,
            lapsa::create_stats_file<MyState>,
    };
    sm.run();
    return 0;
}
