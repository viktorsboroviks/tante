#include <cmath>
#include <deque>
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
const std::string g_reports_results_file_prefix     = iestade::string_from_json(CONFIG_PATH, "reports/results_file_prefix");

std::deque<double> g_training_data;
double g_training_data_min                          = iestade::double_from_json(CONFIG_PATH, "training/data_min");
double g_training_data_max                          = iestade::double_from_json(CONFIG_PATH, "training/data_max");
size_t g_training_data_n                            = iestade::size_t_from_json(CONFIG_PATH, "training/data_n");
size_t g_training_update_n                          = iestade::size_t_from_json(CONFIG_PATH, "training/update_n");
size_t g_training_update_period                     = iestade::size_t_from_json(CONFIG_PATH, "training/update_period");
// clang-format on

class MyState : public lapsa::State {
private:
    std::vector<double> _last_inputs;
    std::vector<double> _last_outputs;

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
        if (!_energy_calculated || _last_inputs[0] != g_training_data[0]) {
            assert(g_training_data.size() == g_training_data_n);
            _energy = 0;
            _last_inputs.clear();
            _last_outputs.clear();
            std::vector<double> correct;
            for (size_t i = 0; i < g_training_data.size(); i++) {
                _last_inputs.push_back(g_training_data[i]);
                std::vector<double> inputs;
                inputs.push_back(g_training_data[i]);
                assert(inputs.size() == net.settings.n_inputs);
                correct.push_back(std::sin(inputs[0]));
                _last_outputs.push_back(net.infer(inputs)[0]);
            }

            _energy = rododendrs::rrmse<std::vector, std::vector>(
                    _last_outputs, correct);
            _energy_calculated = true;
        }
        return _energy;
    }

    void to_csv(const std::string& results_filepath)
    {
        assert(!_last_inputs.empty());
        assert(!_last_outputs.empty());
        assert(_last_inputs.size() == _last_outputs.size());

        std::ofstream f(results_filepath);
        f.is_open();

        // metadata
        f << "# energy: " << get_energy() << std::endl;

        // generate title row
        f << "inference_i,signal_input,signal_output,signal_correct"
          << std::endl;

        // generate content
        for (size_t i = 0; i < _last_inputs.size(); i++) {
            f << i << ",";
            f << std::fixed;
            f << _last_inputs[i] << ",";
            f << _last_outputs[i] << ",";
            f << std::sin(_last_inputs[i]) << std::endl;
        }

        f.close();
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
void init_training_data(lapsa::Context<TState>& c)
{
    (void)c;
    assert(g_training_data.empty());
    for (size_t i = 0; i < g_training_data_n; i++) {
        g_training_data.push_back(rododendrs::rnd_in_range(
                g_training_data_min, g_training_data_max));
    }
    assert(g_training_data.size() == g_training_data_n);
}

template <typename TState>
void update_training_data(lapsa::Context<TState>& c)
{
    if (c.state_i % g_training_update_period) {
        return;
    }

    assert(!g_training_data.empty());
    for (size_t i = 0; i < g_training_update_n; i++) {
        g_training_data.pop_front();
        g_training_data.push_back(rododendrs::rnd_in_range(
                g_training_data_min, g_training_data_max));
    }
    assert(g_training_data.size() == g_training_data_n);
}

template <typename TState>
void create_report_files(lapsa::Context<TState>& c)
{
    if (!c.do_report) {
        return;
    }

    assert(g_reports_dir_name != "");
    assert(g_reports_neurons_file_prefix != "");
    assert(g_reports_connections_file_prefix != "");
    assert(g_reports_results_file_prefix != "");

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
    std::stringstream results_filename;
    results_filename << g_reports_dir_name << "/"
                     << g_reports_results_file_prefix << std::setfill('0')
                     << std::setw(n_states_strlen) << c.state_i << ".csv";

    c.state.net.to_csv(neurons_filename.str(), connections_filename.str());
    c.state.to_csv(results_filename.str());
}

int main()
{
    lapsa::Settings ls{CONFIG_PATH, "lapsa"};
    lapsa::StateMachine<MyState> lsm{ls};
    lsm.init_functions = {
            lapsa::init_log<MyState>,
            lapsa::init_report_linear<MyState>,
            init_training_data<MyState>,
            lapsa::randomize_state<MyState>,
    };
    lsm.init_loop_functions = {
            update_training_data<MyState>,
            lapsa::propose_new_state<MyState>,
            lapsa::record_init_temperature<MyState>,
            lapsa::select_init_temperature_as_max<MyState>,
            lapsa::init_run_progress<MyState>,
            lapsa::check_init_done<MyState>,
    };
    lsm.run_loop_functions = {
            update_training_data<MyState>,
            lapsa::propose_new_state<MyState>,
            lapsa::record_energy<MyState>,
            lapsa::decide_to_cool_sma<MyState>,
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
