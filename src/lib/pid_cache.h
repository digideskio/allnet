/* pid_cache.h */
/* keep track of packet IDs in a collection of bloom filters, the oldest
 * of which can be periodically discarded. */

#ifndef PID_CACHE_H
#define PID_CACHE_H

#include "packet.h"

#define PID_MESSAGE_FILTER  	0
#define PID_ACK_FILTER      	1
#define PID_FILTER_SELECTORS	(PID_ACK_FILTER + 1)

#define PID_SIZE		MESSAGE_ID_SIZE

/* id should refer to at least 16 bytes, and PID_SIZE should be 16 or more
 * filter_selector should be 1 for acks, 0 for messages
 * return 1 if the id (of size FILTER_BITS/8 * FILTER_DEPTH) is found
 * in one of the filters. */
extern int pid_is_in_bloom (const char * id, int filter_selector);

/* id should refer to at least 16 bytes, and PID_SIZE should be 16 or more
 * filter_selector should be 1 for acks, 0 for messages
 * adds the ID to the newest bloom filter.
 * recommend calling is_in_bloom with the id before calling add_to_bloom */
extern void pid_add_to_bloom (const char * id, int filter_selector);

/* save the bloom filter in a file */
extern void pid_save_bloom (void);

/* discard the oldest bloom filter and create a new empty bloom filter */
extern void pid_advance_bloom (void);

#endif /* PID_CACHE_H */
