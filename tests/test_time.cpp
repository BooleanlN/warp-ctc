#include <cmath>
#include <cstdlib>
#include <random>
#include <tuple>
#include <vector>

#include <chrono>

#include <iostream>

#include <ctc.h>

#include "test.h"

bool run_test(int B, int T, int L, int A, int num_threads) {
    std::mt19937 gen(2);

    std::vector<float> acts = genActs(B * T * A);

    std::vector<std::vector<int>> labels;
    std::vector<int> sizes;

    for (int mb = 0; mb < B; ++mb) {
        labels.push_back(genLabels(A, L));
        sizes.push_back(T);
    }

    std::vector<int> flat_labels;
    std::vector<int> label_lengths;
    for (const auto& l : labels) {
        flat_labels.insert(flat_labels.end(), l.begin(), l.end());
        label_lengths.push_back(l.size());
    }

    std::vector<float> costs(B);

    std::vector<float> grads(acts.size());

    ctcOptions options{};
    options.loc = CTC_CPU;
    options.num_threads = num_threads;

    size_t cpu_alloc_bytes;
    throw_on_error(get_workspace_size(label_lengths.data(), sizes.data(),
                                     A, sizes.size(), options,
                                     &cpu_alloc_bytes),
                    "Error: get_workspace_size in run_test");
    
    void* ctc_cpu_workspace = malloc(cpu_alloc_bytes);

    auto start = std::chrono::high_resolution_clock::now();
    throw_on_error(compute_ctc_loss(acts.data(), grads.data(),
                                    flat_labels.data(), label_lengths.data(),
                                    sizes.data(),
                                    A,
                                    B,
                                    costs.data(),
                                    ctc_cpu_workspace,
                                    options),
                    "Error: compute_ctc_loss (0) in run_test");
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "compute_ctc_loss elapsed time: " << elapsed.count() * 1000 << " ms\n";

    float cost = std::accumulate(costs.begin(), costs.end(), 0.);

    free(ctc_cpu_workspace);
}

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Arguments: <Batch size> <Time step> <Label length> <Alphabet size>\n";
        return 1;
    }

    int B = atoi(argv[1]);
    int T = atoi(argv[2]);
    int L = atoi(argv[3]);
    int A = atoi(argv[4]);
    std::cout << "Arguments: " \
                << "\nBatch size: " << B \
                << "\nTime step: " << T \
                << "\nLabel length: " << L \
                << "\nAlphabet size: " << A \
                << std::endl;
    
    int num_threads = 1;
    if (argc >= 6) {
        num_threads = atoi(argv[5]);
        std::cout << "Num threads: " << num_threads << std::endl;
    }

    run_test(B, T, L, A, num_threads);
}