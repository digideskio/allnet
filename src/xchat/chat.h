/* format of chat messages */

#ifndef ALLNET_CHAT_H
#define ALLNET_CHAT_H

#include "packet.h"	/* PACKET_ID_SIZE */

/* messages are indexed by a 64-bit counter, which must be forever unique over
 * all messages between a given pair of sender and receiver, or for a given
 * sender and group.  Messages also have a date/time and timezone.
 *
 * the first counter in any exchange has the value 1.
 *
 * a repeated counter value with a later timestamp means a correction
 * or deletion is requested for a previously sent message.
 *
 * control messages have a counter value of 0xffffffffffffffff, followed
 * by the control message header.
 *
 * the time is encoded as
 * 6 bytes of seconds since midnight GMT on 01/01/2000,
 * 2 bytes of +- minute offset from GMT, set to 0x8000 if not known
 *      (in which case local time is sent)
 *
 * all these values are sent in big-endian order, that is, with the
 * most significant byte first, so counter [0], timestamp [0] and timestamp [6]
 * each have the most significant byte of their value.
 *
 * like any data message, the first PACKET_ID_SIZE of any message are the
 * random bytes which hash to the packet ID sent in the clear (only the
 * first PACKET_ID_SIZE of the sha512 are sent).
 *
 */

/* the chat descriptor is sent before the user text, in the first
 * 32 bytes of the data of the AllNet packet */

#define COUNTER_SIZE	8
#define TIMESTAMP_SIZE	8

struct chat_descriptor {
  unsigned char packet_id [PACKET_ID_SIZE];
  unsigned char counter   [  COUNTER_SIZE];
  unsigned char timestamp [TIMESTAMP_SIZE];
};

#define CHAT_DESCRIPTOR_SIZE	(sizeof (struct chat_descriptor))

#define COUNTER_FLAG	0xffffffffffffffffLL

struct chat_control {
  unsigned char packet_id [PACKET_ID_SIZE];
  unsigned char counter   [  COUNTER_SIZE];  /* always COUNTER_FLAG */
  unsigned char type;
};

#define CHAT_CONTROL_TYPE_REQUEST	1
#define CHAT_CONTROL_TYPE_FRAGMENT	2

/* either of the headers below may immediately follow the chat_control header */
struct chat_control_request {
  unsigned char packet_id [PACKET_ID_SIZE];
  unsigned char counter   [  COUNTER_SIZE];  /* always COUNTER_FLAG */
  unsigned char type;
  unsigned char num_singles;
  unsigned char num_ranges;
  unsigned char padding [5];   /* sent as zeros, ignored on receipt */
  unsigned char last_received [COUNTER_SIZE];
  unsigned char counters  [0];
  /* really, counters has COUNTER_SIZE * (num_singles + 2 * num_ranges) bytes */
  /* this packet requests delivery of all packets that fit one or more of:
     - counter value > last_received
     - counter value listed in one of the first num_singles counters
     - for each pair (start, finish) of counters after the first num_singles,
       start <= counter <= finish */
  /* for example, if num_singles is 3, num_ranges is 1, last_received is 99,
     counters is { 91, 89, 95, 77, 82 }, and the sender has sent up to
     counter 105, it is hereby invited to resend:
     77, 78, 79, 80, 81, 82           (listed in the range)
     89, 91, 95                       (listed as single counter values)
     100, 101, 102, 103, 104, and 105 (implied by last_received)
   */
};

struct chat_control_fragment {
  unsigned char to_be_defined;
};
#endif /* ALLNET_CHAT_H */