#pragma once

#include "GodotNetworkingSockets.h"

namespace VOIP {
	// Opus requires 5, 10, 20, 40 or 60ms frames at 48000Hz
	constexpr uint32_t SAMPLE_RATE = 48000;
	constexpr uint32_t FRAME_SIZE_MS = 20;
	constexpr uint32_t FRAME_SIZE = SAMPLE_RATE * FRAME_SIZE_MS / 1000;
	constexpr uint32_t SEND_BUFFER_SIZE = FRAME_SIZE * 8;
	constexpr uint32_t PACKET_SIZE = sizeof(uint16_t) + 2 * sizeof(uint32_t) + FRAME_SIZE * sizeof(float); // Packet id, client id, voip id, frame data

	void init();
	void send(const AudioFrame *data, uint32_t size);
};
