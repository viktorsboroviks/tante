#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

const size_t N_RUNS = 1000000;

typedef std::function<double(int)> run_function_t;

double run_tanh(double in)
{
    return std::tanh(in);
}

double run_sigmoid(double in)
{
    return 1 / (1 + std::exp(-in));
}

// rectified linear unit
double run_relu(double in)
{
    return std::max(0.0, in);
}

double run_gaussian(double in)
{
    return std::exp(std::pow(-in, 2.0));
}

struct Test {
    std::string name;
    run_function_t f;
};

Test tests[] = {
        {"sigmoid ", run_sigmoid},
        {"relu    ", run_relu},
        {"gaussian", run_gaussian},
        {"tanh    ", run_tanh},
};

int main()
{
    // this is needed so the code is not optimized out
    static volatile double run_retval;

    std::cout << N_RUNS << " runs average" << std::endl;
    for (auto t : tests) {
        size_t total_runtime_ns = 0;
        for (size_t i = 0; i < N_RUNS; i++) {
            const double rnd = rand() / (double)rand();
            auto start = std::chrono::steady_clock::now();
            run_retval = t.f(rnd);
            auto finish = std::chrono::steady_clock::now();
            total_runtime_ns +=
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                            finish - start)
                            .count();
        }
        const double avg_runtime_ns = total_runtime_ns / (double)N_RUNS;
        std::cout << t.name << " " << avg_runtime_ns << "ns" << std::endl;
    }

    (void)run_retval;
    return 0;
}
