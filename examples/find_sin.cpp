#include <cmath>
#include <filesystem>

#include "garaza.hpp"
#include "iestade.hpp"
#include "lapsa.hpp"
#include "rododendrs.hpp"
#include "tante.hpp"

// clang-format off
const std::string CONFIG_PATH                       = "examples/find_sin_config.json";
tante::Settings g_ts(CONFIG_PATH, "tante");

const std::string g_reports_dir_name                = iestade::string_from_json(CONFIG_PATH, "reports/dir_name");
const std::string g_reports_neurons_file_prefix     = iestade::string_from_json(CONFIG_PATH, "reports/neurons_file_prefix");
const std::string g_reports_connections_file_prefix = iestade::string_from_json(CONFIG_PATH, "reports/connections_file_prefix");
const std::string g_reports_signals_file_prefix     = iestade::string_from_json(CONFIG_PATH, "reports/signals_file_prefix");
// clang-format on

class MyState : public lapsa::State {
public:
    tante::Network net;

    MyState(lapsa::Settings& in_settings) :
        State(in_settings),
        net{g_ts}
    {
    }

    double get_energy()
    {
        // if energy not calculated, do it now and store the result
        double prev_training_data = 0;
        if (!_energy_calculated) {
            std::vector<double> inputs;
            const double training_data = rododendrs::rnd01() * 1000.0;
            assert(training_data != prev_training_data);
            prev_training_data = training_data;
            inputs.push_back(training_data);
            assert(inputs.size() == net.settings.n_inputs);
            const std::vector<double> outputs = net.infer(inputs);
            assert(outputs.size() == net.settings.n_outputs);
            const double result = outputs[0];
            _energy             = std::abs(std::sin(training_data) - result);
            _energy_calculated  = true;
        }
        return _energy;
    }

    void randomize()
    {
        net.restore_randomly();
        reset_energy();
    }

    void change()
    {
        while (!net.apply_operation(net.get_random_operation()));
        net.restore_randomly();
        reset_energy();
    }
};

template <typename TState>
void create_report_files(lapsa::Context<TState>& c)
{
    if (!c.do_report) {
        return;
    }

    assert(g_reports_dir_name != "");
    assert(g_reports_neurons_file_prefix != "");
    assert(g_reports_connections_file_prefix != "");

    if (!std::filesystem::exists(g_reports_dir_name)) {
        std::filesystem::create_directory(g_reports_dir_name);
    }
    assert(std::filesystem::exists(g_reports_dir_name));

    const size_t n_states_strlen =
            std::to_string(c.settings.n_states).length();

    std::stringstream neurons_filename;
    neurons_filename << g_reports_dir_name << "/"
                     << g_reports_neurons_file_prefix << std::setfill('0')
                     << std::setw(n_states_strlen) << c.state_i << ".csv";
    std::stringstream connections_filename;
    connections_filename << g_reports_dir_name << "/"
                         << g_reports_connections_file_prefix
                         << std::setfill('0') << std::setw(n_states_strlen)
                         << c.state_i << ".csv";
    std::stringstream signals_filename;
    signals_filename << g_reports_dir_name << "/"
                     << g_reports_signals_file_prefix << std::setfill('0')
                     << std::setw(n_states_strlen) << c.state_i << ".csv";

    c.state.net.to_csv(neurons_filename.str(),
                       connections_filename.str(),
                       signals_filename.str());
}

int main()
{
    lapsa::Settings ls{CONFIG_PATH, "lapsa"};
    lapsa::StateMachine<MyState> lsm{ls};
    lsm.init_functions = {
            lapsa::init_log<MyState>,
            lapsa::init_report_linear<MyState>,
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
            lapsa::decide_to_report<MyState>,
            create_report_files<MyState>,
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
