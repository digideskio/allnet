/* track.h: keep track of how much bandwidth is going to each destination */

#ifndef TRACK_H
#define TRACK_H

/* record that this source is sending this packet of given size */
/* return an integer, as a fraction of MAX_PRIORITY, to indicate what
 * fraction of the available bandwidth this source is using.
 * MAX_PRIORITY is defined in priority.h
 */
extern int track_rate (char * src, int sbits, int packet_size);

/* return the rate of the sender that is sending the most at this time */
/* used by default when we cannot prove who the sender is */
extern int largest_rate ();

#endif /* TRACK_H */
