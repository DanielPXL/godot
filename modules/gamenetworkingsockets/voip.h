#ifndef GODOTNETWORKINGSOCKETS_VOIP_H
#define GODOTNETWORKINGSOCKETS_VOIP_H

#include "GodotNetworkingSockets.h"
#include "audio_stream_netreceive.h"
#include "opus.h"

namespace VOIP {
	// Opus requires 5, 10, 20, 40 or 60ms frames at 48000Hz
	constexpr uint32_t SAMPLE_RATE = 48000;
	constexpr uint32_t FRAME_SIZE_MS = 20;
	constexpr uint32_t FRAME_SIZE = SAMPLE_RATE * FRAME_SIZE_MS / 1000;
	constexpr uint32_t SEND_BUFFER_SIZE = FRAME_SIZE * 8;
	constexpr uint32_t MAX_ENCODED_SIZE = 4000; // 4000 bytes recommended by Opus
	constexpr uint32_t PACKET_HEADER_SIZE = sizeof(uint16_t) + 2 * sizeof(uint32_t) + sizeof(int64_t); // Packet id, client id, voip id, timestamp

	void init();
	void send(const AudioFrame *data, uint32_t size);
	void receive(uint16_t clientId, uint32_t packetId, int64_t timestamp, const void *data, uint32_t size);

	void RegisterAudioReceiver(uint16_t clientId, AudioStreamPlaybackNetReceive *receiver);
	void UnregisterAudioReceiver(uint16_t clientId);

	struct AudioReceiver {
		AudioStreamPlaybackNetReceive *stream = nullptr;
		OpusDecoder *decoder = nullptr;
		uint32_t lastPacketId = 0;
	};
};

#endif // GODOTNETWORKINGSOCKETS_VOIP_H
