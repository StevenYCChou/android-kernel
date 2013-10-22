#ifndef _NETLOCK
#define _NETLOCK

enum __netlock_t {
	NET_LOCK_N, /* Placeholder for no lock */
	NET_LOCK_R, /* Indicates regular lock */
	NET_LOCK_E, /* Indicates exclusive lock */
};
typedef enum __netlock_t netlock_t;

int netlock_release(void);
#endif
