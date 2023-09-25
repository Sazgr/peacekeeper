#ifndef PEACEKEEPER_TIMER
#define PEACEKEEPER_TIMER

#include "typedefs.h"
#include <atomic>
#include <chrono>

class Timer {
private:
    using clock_t = std::chrono::steady_clock;
    using second_t = std::chrono::duration<double, std::ratio<1>>;
    std::chrono::time_point<clock_t> m_beg;
public:
    Timer() : m_beg(clock_t::now()) {}
    Timer(const Timer& rhs) = default;
    Timer& operator=(Timer& rhs) = default;
    void reset() {
        m_beg = clock_t::now();
    }
    double elapsed() const {
        return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
    }
};

class Stop_timer {
private:
    Timer timer;
public:
    std::atomic<bool> stop;
    int hard_time_limit;
    int soft_time_limit;
    u64 hard_nodes_limit;
    u64 soft_nodes_limit;
    int depth_limit;
    Stop_timer(int ht = 0, int st = 0, u64 hn = 0, u64 sn = 0, int d = 0) {
        reset(ht, st, hn, sn, d);
    }
    Stop_timer(const Stop_timer& rhs) = default;
    Stop_timer& operator=(Stop_timer& rhs) = default;
    inline void reset(int ht = 0, int st = 0, u64 hn = 0, u64 sn = 0, int d = 0) {
        stop = false;
        hard_time_limit = ht;
        soft_time_limit = st;
        hard_nodes_limit = hn;
        soft_nodes_limit = sn;
        depth_limit = d;
        timer.reset();
    }
    inline bool stopped() {
        return stop;
    }
    inline bool check(u64 nodes = 0, int depth = 0, bool use_soft_limit = false, double scale = 1.0) {
        if (stop) return true;
        if (hard_time_limit) stop = stop || (static_cast<int>(elapsed() * 1000) >= hard_time_limit);
        if (soft_time_limit && use_soft_limit) stop = stop || (static_cast<int>(elapsed() * 1000 / scale) >= soft_time_limit);
        if (hard_nodes_limit) stop = stop || (nodes >= hard_nodes_limit);
        if (soft_nodes_limit && use_soft_limit) stop = stop || (nodes >= soft_nodes_limit);
        if (depth_limit) stop = stop || (depth > depth_limit);
        return stop;
    }
    inline double elapsed() {
        return timer.elapsed();
    }
};

#endif
