#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <cstdlib>
#include <cstdint>
#include <climits>

/************
 * kt_for() *
 ************/

struct kt_for_t;

typedef struct {
    struct kt_for_t* t;
    long i;
} ktf_worker_t;

typedef struct kt_for_t {
    int n_threads;
    long n;
    ktf_worker_t* w;
    std::function<void(void*, long, int)> func;
    void* data;
} kt_for_t;

void kt_for_single(int n_threads, void (*func)(void*, long, int), void* data, long n) {
    for (int i = 0; i < n; i++) {
        func(data, i, 0);
    }
}

/*****************
 * kt_pipeline() *
 *****************/

struct ktp_t;

typedef struct {
    struct ktp_t* pl;
    int64_t index;
    int step;
    void* data;
} ktp_worker_t;

typedef struct ktp_t {
    void* shared;
    std::function<void*(void*, int, void*)> func;
    int64_t index;
    int n_workers, n_steps;
    std::vector<ktp_worker_t> workers;
    std::mutex mtx;
    std::condition_variable cv;
} ktp_t;

static void ktp_worker_single(ktp_t* p) {
    int step = 0;
    void* data = 0;
    while (step < p->n_steps) {
        data = p->func(p->shared, step, step ? data : 0);
        step = (step == p->n_steps - 1 || data) ? (step + 1) % p->n_steps : p->n_steps;
    }
}

void kt_pipeline_single(int n_threads, void* (*func)(void*, int, void*), void* shared_data, int n_steps) {
    ktp_t aux;
    aux.n_steps = n_steps;
    aux.func = func;
    aux.shared = shared_data;
    ktp_worker_single(&aux);
}

void ktp_worker(ktp_worker_t* w) {
    ktp_t* p = w->pl;
    while (w->step < p->n_steps) {
        std::unique_lock<std::mutex> lock(p->mtx);
        while (true) {
            bool can_start = true;
            for (int i = 0; i < p->n_workers; ++i) {
                if (w == &p->workers[i]) continue; 
                if (p->workers[i].step <= w->step && p->workers[i].index < w->index) {
                    can_start = false;
                    break;
                }
            }
            if (can_start) break;
            p->cv.wait(lock);
        }
        lock.unlock();

        w->data = p->func(p->shared, w->step, w->step ? w->data : nullptr);

        lock.lock();
        w->step = (w->step == p->n_steps - 1 || w->data) ? (w->step + 1) % p->n_steps : p->n_steps;
        if (w->step == 0) w->index = p->index++;
        p->cv.notify_all();
        lock.unlock();
    }
}

extern "C" void kt_pipeline(int n_threads, void* (*func)(void*, int, void*), void* shared_data, int n_steps) {
    ktp_t aux;
    if (n_threads < 1) n_threads = 1;

    aux.n_workers = n_threads;
    aux.n_steps = n_steps;
    aux.func = func;
    aux.shared = shared_data;
    aux.index = 0;
    aux.workers.resize(n_threads);

    for (int i = 0; i < n_threads; ++i) {
        ktp_worker_t& w = aux.workers[i];
        w.step = 0;
        w.pl = &aux;
        w.data = nullptr;
        w.index = aux.index++;
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < n_threads; ++i) {
        threads.emplace_back(ktp_worker, &aux.workers[i]);
    }

    for (auto& t : threads) {
        t.join();
    }
}

