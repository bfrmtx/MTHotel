#ifndef BTHREAD_H
#define BTHREAD_H
#include <vector>
#include <thread>

std::vector<size_t> mk_mini_threads(const size_t nthreads_wanted, const size_t nprocesses_pending) {
    std::vector<size_t> execs;
    size_t nt = std::thread::hardware_concurrency();
    if (nthreads_wanted < nt) nt = nthreads_wanted;
    if (!nthreads_wanted) nt = std::thread::hardware_concurrency() / 2;
    size_t mt = (nprocesses_pending / nt);
    size_t ft = nprocesses_pending % nt;
    for (size_t i = 0; i < mt; ++i) {
        execs.emplace_back(nt);
    }
    if (ft) execs.emplace_back(ft);

    return execs;
}

#endif // BTHREAD_H
