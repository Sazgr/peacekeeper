#ifndef PEACEKEEPER_SPSA
#define PEACEKEEPER_SPSA

#include <array>

#ifdef SPSA
#define spsa
#else
#define spsa constexpr
#endif

spsa std::array<int, 6> futile_margins{47, 75, 98, 119, 138, 156};
constexpr std::array<int, 3> aspiration_bounds{28, 90, 280};
spsa std::array<double, 4> tc_stability{2.05, 1.20, 0.90, 0.85};

spsa double futility_multiplier = 47.5;
spsa double futility_power = 0.666;
spsa double see_noisy_constant = 113.5;
spsa double see_noisy_linear = 0.0;
spsa double see_noisy_quadratic = 8.0;
spsa double see_quiet_constant = 90.0;
spsa double see_quiet_linear = 10.0;
spsa double see_quiet_quadratic = 8.8;
spsa double node_timescale_base = 1.8;
spsa double node_timescale_div = 1.35;
spsa double aspiration_beta_timescale = 1.35;

#endif
