#ifndef CONCURRENT_ADDER_H
#define CONCURRENT_ADDER_H

#include <vector>
#include <stack>
#include <atomic>
#include <mutex>

namespace concurrent {

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
class Adder {
    class PerThread {
    public:
        PerThread() : value(0), next(nullptr), ref_count(2) {} // ref = {Adder, ObjectAggregate}
        PerThread(std::atomic<PerThread*>& head) : value(0), next(nullptr), ref_count(2) {
            PerThread* senti_next;
            do {
                senti_next = head.load();        
                this->next.store(senti_next);
            } while (!head.compare_exchange_strong(senti_next, this));
        }

        void add(Value v) {
            value.store(value.load() + v);
        }
        void reset() {
            value.store(0);
        }
        static PerThread* create(std::atomic<PerThread*>& head) {
            return new PerThread(head);
        }
        void release() {
            if(--ref_count == 0) {
                delete this;
            }
        }
    public:
        std::atomic<PerThread*> next;
        std::atomic<Value> value;
        std::atomic<int>  ref_count;
    };

    class ObjectAggregate {
    public:
        ~ObjectAggregate() {
            for(auto p : handles) {
                p->release();
            }
        }
        PerThread* get_instance(Adder* adder) {
            while(handles.size() <= adder->id) {
                handles.push_back(PerThread::create(adder->head));
            }
            return handles[adder->id];
        }
    private:
        std::vector<PerThread*> handles;
    };


public:
    Adder() : value(0), head(nullptr) {
        id = id_allocator.allocate();
    }
    ~Adder() {
        auto p = head.load();
        while(p != nullptr) {
            auto next = p->next.load();
            p->reset();
            p->release();
            p = next;
        }
        id_allocator.deallocate(id);
    }
    
    void add(Value value) {
        thread_local ObjectAggregate aggregate;
        aggregate.get_instance(this)->add(value);
    }

    Value sum() {
	    Value sum_value = value.load();
        auto next = head.load();
        size_t i = 0;
        while(next != nullptr) {
            sum_value += next->value.load();
            next = next->next.load();
        }
        return sum_value;
    }

private:
    std::atomic<PerThread*> head;
    std::atomic<Value> value;
    size_t id;
    static IDAllocator id_allocator;
};

template<typename Value>
IDAllocator Adder<Value>::id_allocator;
}

#endif