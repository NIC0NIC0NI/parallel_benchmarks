#ifdef OMP

#include <omp.h>
#include "thread_test_utils.h"

int get_num_threads() {
    int nthreads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        nthreads = omp_get_num_threads();
    }
    return nthreads;
}

void parallel_impl(task& t) {
    #pragma omp parallel
    {
        t.run();
    }
}
#elif defined(TBB)

#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include "thread_test_utils.h"

int get_num_threads() {
    return tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
}

void parallel_impl(task& t) {
    tbb::parallel_for(0, get_num_threads(), [&t] (int i) {
        t.run();
    });
}

#endif
