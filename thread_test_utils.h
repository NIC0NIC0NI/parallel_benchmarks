#ifndef THREAD_TEST_UTILS
#define THREAD_TEST_UTILS

class task {
public:
    virtual void run() = 0;
};

template<typename Function>
class function_task : public task {
public:
    function_task(const Function& func) : f(func) {}
    void run() override {
        f();
    }
private:
    const Function& f;
};


int get_num_threads();
void parallel_impl(task& t);

template<typename Function>
void parallel(const Function& func) {
    function_task<Function> t(func);
    parallel_impl(t);
}


#endif