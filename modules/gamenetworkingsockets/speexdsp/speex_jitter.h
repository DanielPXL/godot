/* Copyright (C) 2002 Jean-Marc Valin */
/**
   @file speex_jitter.h
   @brief Adaptive jitter buffer for Speex
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef SPEEX_JITTER_H
#define SPEEX_JITTER_H
/** @defgroup JitterBuffer JitterBuffer: Adaptive jitter buffer
 *  This is the jitter buffer that reorders UDP/RTP packets and adjusts the buffer size
 * to maintain good quality and low latency.
 *  @{
 */

#include "speexdsp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPEEX_JITTER_MAX_BUFFER_SIZE 200 /**< Maximum number of packets in jitter buffer */

#define MAX_TIMINGS 40
#define MAX_BUFFERS 3
#define TOP_DELAY 40


	/** Buffer that keeps the time of arrival of the latest packets */
struct TimingBuffer {
	int filled; /**< Number of entries occupied in "timing" and "counts"*/
	int curr_count; /**< Number of packet timings we got (including those we discarded) */
	spx_int32_t timing[MAX_TIMINGS]; /**< Sorted list of all timings ("latest" packets first) */
	spx_int16_t counts[MAX_TIMINGS]; /**< Order the packets were put in (will be used for short-term estimate) */
};
	
/** Definition of an incoming packet */
struct _JitterBufferPacket {
	char *data; /**< Data bytes contained in the packet */
	spx_uint32_t len; /**< Length of the packet in bytes */
	spx_uint32_t timestamp; /**< Timestamp for the packet */
	spx_uint32_t span; /**< Time covered by the packet (same units as timestamp) */
	spx_uint16_t sequence; /**< RTP Sequence number if available (0 otherwise) */
	spx_uint32_t user_data; /**< Put whatever data you like here (it's ignored by the jitter buffer) */
};

/** Definition of an incoming packet */
typedef struct _JitterBufferPacket JitterBufferPacket;

/** Generic adaptive jitter buffer state */
/** Jitter buffer structure */
struct JitterBuffer_ {
	spx_uint32_t pointer_timestamp; /**< Timestamp of what we will *get* next */
	spx_uint32_t last_returned_timestamp; /**< Useful for getting the next packet with the same timestamp (for fragmented media) */
	spx_uint32_t next_stop; /**< Estimated time the next get() will be called */

	spx_int32_t buffered; /**< Amount of data we think is still buffered by the application (timestamp units)*/

	JitterBufferPacket packets[SPEEX_JITTER_MAX_BUFFER_SIZE]; /**< Packets stored in the buffer */
	spx_uint32_t arrival[SPEEX_JITTER_MAX_BUFFER_SIZE]; /**< Packet arrival time (0 means it was late, even though it's a valid timestamp) */

	void (*destroy)(void *); /**< Callback for destroying a packet */

	spx_int32_t delay_step; /**< Size of the steps when adjusting buffering (timestamp units) */
	spx_int32_t concealment_size; /**< Size of the packet loss concealment "units" */
	int reset_state; /**< True if state was just reset        */
	int buffer_margin; /**< How many frames we want to keep in the buffer (lower bound) */
	int late_cutoff; /**< How late must a packet be for it not to be considered at all */
	int interp_requested; /**< An interpolation is requested by speex_jitter_update_delay() */
	int auto_adjust; /**< Whether to automatically adjust the delay at any time */

	struct TimingBuffer _tb[MAX_BUFFERS]; /**< Don't use those directly */
	struct TimingBuffer *timeBuffers[MAX_BUFFERS]; /**< Storing arrival time of latest frames so we can compute some stats */
	int window_size; /**< Total window over which the late frames are counted */
	int subwindow_size; /**< Sub-window size for faster computation  */
	int max_late_rate; /**< Absolute maximum amount of late packets tolerable (in percent) */
	int latency_tradeoff; /**< Latency equivalent of losing one percent of packets */
	int auto_tradeoff; /**< Latency equivalent of losing one percent of packets (automatic default) */

	int lost_count; /**< Number of consecutive lost packets  */
};

/** Generic adaptive jitter buffer state */
typedef struct JitterBuffer_ JitterBuffer;

/** Packet has been retrieved */
#define JITTER_BUFFER_OK 0
/** Packet is lost or is late */
#define JITTER_BUFFER_MISSING 1
/** A "fake" packet is meant to be inserted here to increase buffering */
#define JITTER_BUFFER_INSERTION 2
/** There was an error in the jitter buffer */
#define JITTER_BUFFER_INTERNAL_ERROR -1
/** Invalid argument */
#define JITTER_BUFFER_BAD_ARGUMENT -2


/** Set minimum amount of extra buffering required (margin) */
#define JITTER_BUFFER_SET_MARGIN 0
/** Get minimum amount of extra buffering required (margin) */
#define JITTER_BUFFER_GET_MARGIN 1
/* JITTER_BUFFER_SET_AVAILABLE_COUNT wouldn't make sense */

/** Get the amount of available packets currently buffered */
#define JITTER_BUFFER_GET_AVAILABLE_COUNT 3
/** Included because of an early misspelling (will remove in next release) */
#define JITTER_BUFFER_GET_AVALIABLE_COUNT 3

/** Assign a function to destroy unused packet. When setting that, the jitter
    buffer no longer copies packet data. */
#define JITTER_BUFFER_SET_DESTROY_CALLBACK 4
/**  */
#define JITTER_BUFFER_GET_DESTROY_CALLBACK 5

/** Tell the jitter buffer to only adjust the delay in multiples of the step parameter provided */
#define JITTER_BUFFER_SET_DELAY_STEP 6
/**  */
#define JITTER_BUFFER_GET_DELAY_STEP 7

/** Tell the jitter buffer to only do concealment in multiples of the size parameter provided */
#define JITTER_BUFFER_SET_CONCEALMENT_SIZE 8
#define JITTER_BUFFER_GET_CONCEALMENT_SIZE 9

/** Absolute max amount of loss that can be tolerated regardless of the delay. Typical loss
    should be half of that or less. */
#define JITTER_BUFFER_SET_MAX_LATE_RATE 10
#define JITTER_BUFFER_GET_MAX_LATE_RATE 11

/** Equivalent cost of one percent late packet in timestamp units */
#define JITTER_BUFFER_SET_LATE_COST 12
#define JITTER_BUFFER_GET_LATE_COST 13


/** Initialises jitter buffer
 *
 * @param step_size Starting value for the size of concleanment packets and delay
       adjustment steps. Can be changed at any time using JITTER_BUFFER_SET_DELAY_STEP
       and JITTER_BUFFER_GET_CONCEALMENT_SIZE.
 * @return Newly created jitter buffer state
 */
JitterBuffer *jitter_buffer_init(int step_size);

/** Restores jitter buffer to its original state
 *
 * @param jitter Jitter buffer state
 */
void jitter_buffer_reset(JitterBuffer *jitter);

/** Destroys jitter buffer
 *
 * @param jitter Jitter buffer state
 */
void jitter_buffer_destroy(JitterBuffer *jitter);

/** Put one packet into the jitter buffer
 *
 * @param jitter Jitter buffer state
 * @param packet Incoming packet
*/
void jitter_buffer_put(JitterBuffer *jitter, const JitterBufferPacket *packet);

/** Get one packet from the jitter buffer
 *
 * @param jitter Jitter buffer state
 * @param packet Returned packet
 * @param desired_span Number of samples (or units) we wish to get from the buffer (no guarantee)
 * @param current_timestamp Timestamp for the returned packet
*/
int jitter_buffer_get(JitterBuffer *jitter, JitterBufferPacket *packet, spx_int32_t desired_span, spx_int32_t *start_offset);

/** Used right after jitter_buffer_get() to obtain another packet that would have the same timestamp.
 * This is mainly useful for media where a single "frame" can be split into several packets.
 *
 * @param jitter Jitter buffer state
 * @param packet Returned packet
 */
int jitter_buffer_get_another(JitterBuffer *jitter, JitterBufferPacket *packet);

/** Get pointer timestamp of jitter buffer
 *
 * @param jitter Jitter buffer state
*/
int jitter_buffer_get_pointer_timestamp(JitterBuffer *jitter);

/** Advance by one tick
 *
 * @param jitter Jitter buffer state
*/
void jitter_buffer_tick(JitterBuffer *jitter);

/** Telling the jitter buffer about the remaining data in the application buffer
 * @param jitter Jitter buffer state
 * @param rem Amount of data buffered by the application (timestamp units)
 */
void jitter_buffer_remaining_span(JitterBuffer *jitter, spx_uint32_t rem);

/** Used like the ioctl function to control the jitter buffer parameters
 *
 * @param jitter Jitter buffer state
 * @param request ioctl-type request (one of the JITTER_BUFFER_* macros)
 * @param ptr Data exchanged to-from function
 * @return 0 if no error, -1 if request in unknown
*/
int jitter_buffer_ctl(JitterBuffer *jitter, int request, void *ptr);

int jitter_buffer_update_delay(JitterBuffer *jitter, JitterBufferPacket *packet, spx_int32_t *start_offset);

/* @} */

#ifdef __cplusplus
}
#endif

#endif
