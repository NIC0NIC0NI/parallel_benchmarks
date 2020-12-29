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

#define CACHE_LINE 64

template<typename Value>
class Adder {
    class alignas(CACHE_LINE) PerThread {
    public:
        PerThread(std::atomic<PerThread*>& head) : value(0), next(nullptr), adder_nref(false), aggregate_nref(false) {
            Register(head);
        }
        void Register(std::atomic<PerThread*>& head) {
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
            next.store(nullptr);
        }
        void aggregate_release() {
            aggregate_nref = true;
            if(adder_nref) {
                delete this;
            }
        }
        void adder_release() {
            adder_nref = true;
            if(aggregate_nref) {
                delete this;
            }
        }
    public:
        std::atomic<PerThread*> next;
        std::atomic<Value> value;
        std::atomic<bool>  adder_nref, aggregate_nref; // reference tags
    };

    class ObjectAggregate {
    public:
        ~ObjectAggregate() {
            for(auto p : handles) {
                p->aggregate_release();
            }
        }
        PerThread* get_instance(Adder* adder) {
            while(handles.size() <= adder->id) {
                handles.push_back(new PerThread(adder->head));
            }
            auto h = handles[adder->id];
            if(h->adder_nref.load()) { // this ID is once free'd and allocated again
                h->Register(adder->head);
                h->adder_nref.store(false);
            }
            return h;
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
            p->adder_release();
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