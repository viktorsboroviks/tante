#include <cmath>

#include "garaza.hpp"
#include "lapsa.hpp"
#include "rododendrs.hpp"
#include "tante.hpp"

const std::string CONFIG_PATH = "examples/find_sin_config.json";

tante::Settings g_ts(CONFIG_PATH, "tante");

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
            const double training_data = rododendrs::rnd01() * 10000.0;
            inputs.push_back(training_data);
            assert(inputs.size() == _n.settings.n_inputs);
            const std::vector<double> outputs = _n.infer(inputs);
            assert(outputs.size() == _n.settings.n_outputs);
            // clang-format off
            const double result = outputs[0];
            _energy = std::abs(std::sin(training_data) - result);
            _energy_calculated = true;
            // clang-format on
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

int main()
{
    lapsa::Settings ls{CONFIG_PATH, "lapsa"};
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
