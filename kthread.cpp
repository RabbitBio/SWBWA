#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <pthread.h>
#include <sched.h>
#include <cstring>

#include <atomic>
#include <cassert>
#include <mpi.h>


/************
 * kt_for() *
 ************/

struct kt_for_t;

typedef struct {
    struct kt_for_t *t;
    long i;
} ktf_worker_t;

typedef struct kt_for_t {
    int n_threads;
    long n;
    ktf_worker_t *w;
    void (*func)(void*,long,int);
    void *data;
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
    struct ktp_t *pl;
    int64_t index;
    int step;
    void *data;
} ktp_worker_t;

typedef struct ktp_t {
    void *shared;
    void *(*func)(void*, int, void*);
    int64_t index;
    int n_workers, n_steps;
    ktp_worker_t *workers;
    pthread_mutex_t mutex;
    pthread_cond_t cv;
} ktp_t;


static void ktp_worker_single(ktp_t* p) {
    int step = 0;
    void* data = 0;
    while (step < p->n_steps) {
        data = p->func(p->shared, step, step ? data : 0);
        step = (step == p->n_steps - 1 || data) ? (step + 1) % p->n_steps : p->n_steps;
    }
    p->func(p->shared, 3, 0);
}

extern "C" void kt_pipeline_single(int n_threads, void* (*func)(void*, int, void*), void* shared_data, int n_steps) {
    ktp_t aux;
    aux.n_steps = n_steps;
    aux.func = func;
    aux.shared = shared_data;
    ktp_worker_single(&aux);
}

//void ktp_worker(ktp_worker_t* w) {
//    ktp_t* p = w->pl;
//    while (w->step < p->n_steps) {
//        std::unique_lock<std::mutex> lock(p->mtx);
//        while (true) {
//            bool can_start = true;
//            for (int i = 0; i < p->n_workers; ++i) {
//                if (w == &p->workers[i]) continue;
//                if (p->workers[i].step <= w->step && p->workers[i].index < w->index) {
//                    can_start = false;
//                    break;
//                }
//            }
//            if (can_start) break;
//            p->cv.wait(lock);
//        }
//        lock.unlock();
//
//        w->data = p->func(p->shared, w->step, w->step ? w->data : nullptr);
//
//        lock.lock();
//        w->step = (w->step == p->n_steps - 1 || w->data) ? (w->step + 1) % p->n_steps : p->n_steps;
//        if (w->step == 0) w->index = p->index++;
//        p->cv.notify_all();
//        lock.unlock();
//    }
//}
//
//extern "C" void kt_pipeline(int n_threads, void* (*func)(void*, int, void*), void* shared_data, int n_steps) {
//    ktp_t aux;
//    if (n_threads < 1) n_threads = 1;
//
//    aux.n_workers = n_threads;
//    aux.n_steps = n_steps;
//    aux.func = func;
//    aux.shared = shared_data;
//    aux.index = 0;
//    aux.workers.resize(n_threads);
//
//    for (int i = 0; i < n_threads; ++i) {
//        ktp_worker_t& w = aux.workers[i];
//        w.step = 0;
//        w.pl = &aux;
//        w.data = nullptr;
//        w.index = aux.index++;
//    }
//
//    std::vector<std::thread> threads;
//    for (int i = 0; i < n_threads; ++i) {
//        threads.emplace_back(ktp_worker, &aux.workers[i]);
//    }
//
//    for (auto& t : threads) {
//        t.join();
//    }
//}



using DataType = void*;

const int queue_item_limit = 4;



std::atomic_int queue2_P1;
std::atomic_int queue2_P2;
std::atomic_int queue2_Num;

struct MyQueue {
    DataType* q_data;
    std::atomic_int q_p1;
    std::atomic_int q_p2;
    std::atomic_int q_num;
};

int get_target_cpu() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank;
}

void set_thread_affinity(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    pthread_t thread = pthread_self();
    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        std::cerr << "Error setting CPU affinity: " << strerror(result) << std::endl;
    }
}

std::mutex mtx;

void reader_thread(ktp_t* p, MyQueue& queue, std::atomic<bool>& done_reading) {
    //set_thread_affinity(1);
    DataType data = nullptr;
    while (true) {

        {
            //std::lock_guard<std::mutex> lock(mtx);
            data = p->func(p->shared, 0, nullptr);
        }
        if (data == nullptr) break;

        while (queue.q_num >= queue_item_limit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        queue.q_data[queue.q_p2++] = data;
        queue.q_num++;
        printf("step1 %p\n", data);
    }
    done_reading = true;
}


void processor_thread(ktp_t* p, MyQueue& read_queue, MyQueue& write_queue, std::atomic<bool>& done_reading, std::atomic<bool>& done_processing) {
    //set_thread_affinity(get_target_cpu());
    DataType item = nullptr;
    bool overWhile = 0;
    while (true) {
        while (read_queue.q_num == 0) {
            if (done_reading) {
                overWhile = 1;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (overWhile) break;
        item = read_queue.q_data[read_queue.q_p1++];
        read_queue.q_num--;
        printf("step2 get %p\n", item);

        DataType processed_data = item;
        processed_data = p->func(p->shared, 1, item);
        printf("step2 put %p\n", processed_data);

        while (write_queue.q_num >= queue_item_limit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        write_queue.q_data[write_queue.q_p2++] = processed_data;
        write_queue.q_num++;
    }
    done_processing = true;
}

void writer_thread(ktp_t* p, MyQueue& queue, std::atomic<bool>& done_processing) {
    //set_thread_affinity(4);
    DataType item = nullptr;
    bool overWhile = 0;
    while (true) {
        while (queue.q_num == 0) {
            if (done_processing) {
                overWhile = 1;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (overWhile) break;
        item = queue.q_data[queue.q_p1++];
        queue.q_num--;
        printf("step3 %p\n", item);
        {
            //std::lock_guard<std::mutex> lock(mtx);
            p->func(p->shared, 2, item);
        }
    }
    p->func(p->shared, 3, item);
}

extern "C" void kt_pipeline_queue(int n_threads, void* (*func)(void*, int, void*), void* shared_data, int n_steps) {

    ktp_t aux;
    aux.func = func;
    aux.shared = shared_data;
    aux.n_steps = n_steps;
    assert(n_threads == 3);
    assert(n_steps == 3);

    MyQueue read_queue;
    MyQueue write_queue;
    
    read_queue.q_p1 = 0;
    read_queue.q_p2 = 0;
    read_queue.q_num = 0;
    read_queue.q_data= new DataType[1 << 20];

    write_queue.q_p1 = 0;
    write_queue.q_p2 = 0;
    write_queue.q_num = 0;
    write_queue.q_data = new DataType[1 << 20];

    std::atomic<bool> done_reading{false};
    std::atomic<bool> done_processing{false};

    std::thread reader(reader_thread, &aux, std::ref(read_queue), std::ref(done_reading));
    //std::thread processor(processor_thread, &aux, std::ref(read_queue), std::ref(write_queue), std::ref(done_reading), std::ref(done_processing));
    std::thread writer(writer_thread, &aux, std::ref(write_queue), std::ref(done_processing));

    processor_thread(&aux, read_queue, write_queue, done_reading, done_processing);
    reader.join();
    //processor.join();
    writer.join();


    delete read_queue.q_data;
    delete write_queue.q_data;
}
