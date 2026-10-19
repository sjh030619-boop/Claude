/* Minimal gint stand-in: the timer never fires in the simulator. */
#ifndef SIM_GINT_TIMER_H
#define SIM_GINT_TIMER_H
#define TIMER_ANY (-1)
#define GINT_CALL(f, ...) 0
static inline int timer_configure(int type, unsigned long long delay_us, int call) { (void)type; (void)delay_us; (void)call; return 0; }
static inline void timer_start(int t) { (void)t; }
static inline void timer_stop(int t) { (void)t; }
static inline int timer_timeout(volatile void *arg) { (void)arg; return 0; }
#endif
