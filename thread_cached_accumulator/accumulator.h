#ifndef CONCURRENT_accumulator_H
#define CONCURRENT_accumulator_H

#include <vector>
#include <stack>
#include <atomic>
#include <mutex>

namespace concurrent {

#define CACHE_LINE 64

class IDAllocator {
public:
    IDAllocator() : upper_bound(0) {}
    size_t allocate() {
        std::lock_guard<std::mutex> guard(lock);
        if(freed.empty()) {
            return upper_bound ++;
        } else {
            size_t id = freed.top();
            freed.pop();
            return id;
        }
    }

    void deallocate(size_t id) {
        std::lock_guard<std::mutex> guard(lock);
        freed.push(id);
    }
private:
    size_t upper_bound;
    std::stack<size_t, std::vector<size_t>> freed;

    // TODO: use concurrent_stack or concurrent_queue
    std::mutex lock;
};


template<typename Value>
class Accumulator {
public:
    class alignas(CACHE_LINE) ThreadLocalInstance {
    public:
        template<typename Operator>
        void accumulate(Value v, const Operator& op) {
            value.store(op(value.load(), v));
        }
    private:
        ThreadLocalInstance(std::atomic<ThreadLocalInstance*>& head) : value(0), next(nullptr), ref_count(2) {
            Register(head);
        }
        void Register(std::atomic<ThreadLocalInstance*>& head) {
            ThreadLocalInstance* senti_next;
            do {
                senti_next = head.load();        
                this->next.store(senti_next);
            } while (!head.compare_exchange_strong(senti_next, this));
        }
        void reset() {
            value.store(0);
            next.store(nullptr);
        }
        void release() {
            if((--ref_count) == 0) {
                delete this;
            }
        }
    private:
        friend class ObjectAggregate;
        friend class Accumulator;
        std::atomic<ThreadLocalInstance*> next;
        std::atomic<Value> value;
        std::atomic<int>   ref_count;
    };
private:
    class ObjectAggregate {
    public:
        ~ObjectAggregate() {
            for(auto p : handles) {
                p->release();
            }
        }
        ThreadLocalInstance& get_instance(Accumulator* accumulator) {
            while(handles.size() <= accumulator->id) {
                handles.push_back(new ThreadLocalInstance(accumulator->head));
            }
            auto h = handles[accumulator->id];
            if(h->ref_count.load() == 1) { // this ID is once free'd and allocated again
                h->Register(accumulator->head);
                h->accumulator_nref.store(2);
            }
            return *h;
        }
    private:
        std::vector<ThreadLocalInstance*> handles;
    };
public:
    Accumulator() : value(0), head(nullptr) {
        id = id_allocator.allocate();
    }
    ~Accumulator() {
        auto p = head.load();
        while(p != nullptr) {
            auto next = p->next.load();
            p->reset();
            p->release();
            p = next;
        }
        id_allocator.deallocate(id);
    }
    
    template<typename Operator>
    Value result(Operator& op) {
	    Value sum_value = value.load();
        auto next = head.load();
        while(next != nullptr) {
            sum_value = op(sum_value, next->value.load());
            next = next->next.load();
        }
        return sum_value;
    }

    ThreadLocalInstance& get_instance() {
        thread_local ObjectAggregate aggregate;
        return aggregate.get_instance(this);
    }

private:
    std::atomic<ThreadLocalInstance*> head;
    std::atomic<Value> value;
    size_t id;
    static IDAllocator id_allocator;
};

template<typename Value>
IDAllocator Accumulator<Value>::id_allocator;
}

#endif