#ifndef PEACEKEEPER_TIMER
#define PEACEKEEPER_TIMER

#include "typedefs.h"
#include <atomic>
#include <chrono>

class Timer {
private:
	using clock_t = std::chrono::steady_clock;
	using second_t = std::chrono::duration<double, std::ratio<1> >;	
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
    int time_limit;
    bool time_limit_active;
    u64 nodes_limit;
    bool nodes_limit_active;
    int depth_limit;
    bool depth_limit_active;
    Stop_timer(int t=0, u64 n=0, int d=0) {
        stop = false;
        time_limit_active = (t != 0);
        time_limit = t;
        nodes_limit_active = (n != 0);
        nodes_limit = n;
        depth_limit_active = (d != 0);
        depth_limit = d;
        timer.reset();
    }
    void reset(int t=0, u64 n=0, int d=0) {
        stop = false;
        time_limit_active = (t != 0);
        time_limit = t;
        nodes_limit_active = (n != 0);
        nodes_limit = n;
        depth_limit_active = (d != 0);
        depth_limit = d;
        timer.reset();
    }
    bool check(u64 nodes = 0, int depth = 0) {
        if (stop) return true;
        if (time_limit_active) stop = stop || (static_cast<int>(elapsed() * 1000) >= time_limit);
        if (nodes_limit_active) stop = stop || (nodes >= nodes_limit);
        if (depth_limit_active) stop = stop || (depth > depth_limit);
        return stop;
    }
    int percent_time() {
        return (time_limit_active ? static_cast<int>(elapsed() * 100000 / time_limit) : 0);
    }
    double elapsed() {
        return timer.elapsed();
    }
};

#endif
