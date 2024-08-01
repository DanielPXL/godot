#include "voip.h"
#include "circular_buffer.h"
#include "core/os/time.h"
#include <chrono>
#include <thread>

Thread sendThread;
CircularBuffer<float> sendBuffer = CircularBuffer<float>(VOIP::SEND_BUFFER_SIZE);
Mutex sendBufferMutex;
float *workingBuffer = new float[VOIP::FRAME_SIZE];
void *packetBuffer = new uint8_t[VOIP::PACKET_HEADER_SIZE + VOIP::MAX_ENCODED_SIZE];
uint32_t sendVoipId = 1;
OpusEncoder *encoder = nullptr;
HashMap<uint16_t, VOIP::AudioReceiver*> audioReceivers;
float *decodeBuffer = new float[VOIP::FRAME_SIZE];

void SendThread(void *args) {
	while (true) {
		sendBufferMutex.lock();
		while (sendBuffer.health() >= VOIP::FRAME_SIZE) {
			for (int i = 0; i < VOIP::FRAME_SIZE; i++) {
				workingBuffer[i] = sendBuffer.pop();
			}
			sendBufferMutex.unlock();

			*(uint16_t *)((uint8_t *)packetBuffer + 0) = 1; // Packet id for voip data
			*(uint32_t *)((uint8_t *)packetBuffer + sizeof(uint16_t)) = 0; // User ID (filled in by server)
			*(uint32_t *)((uint8_t *)packetBuffer + sizeof(uint16_t) + sizeof(uint32_t)) = sendVoipId; // VOIP ID
			*(int64_t *)((uint8_t *)packetBuffer + sizeof(uint16_t) + 2 * sizeof(uint32_t)) = Time::get_singleton()->get_ticks_usec(); // Timestamp

			sendVoipId++;

			uint8_t *data = (uint8_t *)packetBuffer + VOIP::PACKET_HEADER_SIZE;
			int32_t size = opus_encode_float(encoder, workingBuffer, VOIP::FRAME_SIZE, data, VOIP::MAX_ENCODED_SIZE);

			GodotNetClient::SendRaw(packetBuffer, size, k_nSteamNetworkingSend_UnreliableNoDelay, 0);

			sendBufferMutex.lock();
		}
		sendBufferMutex.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(VOIP::FRAME_SIZE_MS / 2));
	}
}

void VOIP::init() {
	if (AudioDriver::get_singleton()->get_mix_rate() != SAMPLE_RATE) {
		print_error("[VOIP] Audio driver mix rate must be 48000Hz");
		return;
	}

	int error;
	encoder = opus_encoder_create(SAMPLE_RATE, 1, OPUS_APPLICATION_VOIP, &error);
	if (error != OPUS_OK) {
		print_error("[VOIP] Failed to create Opus encoder: " + String::num(error));
		return;
	}

	sendThread.start(SendThread, nullptr);
}

void VOIP::send(const AudioFrame *data, uint32_t size) {
	sendBufferMutex.lock();
	for (uint32_t i = 0; i < size; i++) {
		float sample = (data[i].l + data[i].r) / 2.0f;
		sendBuffer.push(sample);
	}
	sendBufferMutex.unlock();
}

void VOIP::receive(uint16_t clientId, uint32_t packetId, int64_t timestamp, const void* data, uint32_t size) {
	if (!audioReceivers.has(clientId)) {
		print_line(String("[VOIP] No audio receiver registered for client ") + String::num(clientId));
		return;
	}

	AudioReceiver* receiver = audioReceivers[clientId];

	while (receiver->lastPacketId < packetId - 1) {
		print_line("[VOIP] Received out-of-order packet");
		opus_decode_float(receiver->decoder, nullptr, 0, decodeBuffer, VOIP::FRAME_SIZE, 0);
		receiver->lastPacketId++;
		receiver->stream->add_buffer(decodeBuffer, VOIP::FRAME_SIZE * sizeof(float));
	}

	opus_decode_float(receiver->decoder, (uint8_t *)data, size, decodeBuffer, VOIP::FRAME_SIZE, 0);
	receiver->lastPacketId = packetId;

	receiver->stream->add_buffer(decodeBuffer, VOIP::FRAME_SIZE * sizeof(float));
}

void VOIP::RegisterAudioReceiver(uint16_t clientId, AudioStreamPlaybackNetReceive *stream) {
	if (audioReceivers.has(clientId)) {
		print_error(String("[VOIP] Audio receiver already registered for client") + String::num(clientId));
		return;
	}

	int error;
	OpusDecoder *decoder = opus_decoder_create(SAMPLE_RATE, 1, &error);
	if (error != OPUS_OK) {
		print_error("[VOIP] Failed to create Opus decoder: " + String::num(error));
		return;
	}

	AudioReceiver *receiver = new AudioReceiver{ stream, decoder };
	audioReceivers[clientId] = receiver;
}

void VOIP::UnregisterAudioReceiver(uint16_t clientId) {
	if (!audioReceivers.has(clientId)) {
		print_error(String("[VOIP] No audio receiver registered for client") + String::num(clientId));
		return;
	}

	opus_decoder_destroy(audioReceivers[clientId]->decoder);
	delete audioReceivers[clientId];
	audioReceivers.erase(clientId);
}
