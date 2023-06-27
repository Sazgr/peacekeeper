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
    u64 nodes_limit;
    int depth_limit;
    Stop_timer(int ht=0, int st=0, u64 n=0, int d=0) {
        reset(ht, st, n, d);
    }
    inline void reset(int ht=0, int st = 0, u64 n=0, int d=0) {
        stop = false;
        hard_time_limit = ht;
        soft_time_limit = st;
        nodes_limit = n;
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
        if (nodes_limit) stop = stop || (nodes >= nodes_limit);
        if (depth_limit) stop = stop || (depth > depth_limit);
        return stop;
    }
    inline double elapsed() {
        return timer.elapsed();
    }
};

#endif
