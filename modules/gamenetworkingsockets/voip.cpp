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
Thread decoderThread;
HashMap<uint32_t, VOIP::AudioReceiver *> audioReceivers;
Mutex audioReceiversMutex;
float *decodeBuffer = new float[VOIP::FRAME_SIZE];

float *silence = new float[VOIP::FRAME_SIZE];

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
			*(uint64_t *)((uint8_t *)packetBuffer + sizeof(uint16_t) + 2 * sizeof(uint32_t)) = Time::get_singleton()->get_ticks_msec(); // Timestamp

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

void DecoderThread(void *args) {
	while (true) {
		audioReceiversMutex.lock();
		for (auto &receiver : audioReceivers) {
			if (receiver.value->stream->frames_available_read() > receiver.value->stream->get_size() / 2) {
				continue;
			}

			jitter_buffer_tick(receiver.value->jitterBuffer);
			JitterBufferPacket jitterPacket;
			int result = jitter_buffer_get(receiver.value->jitterBuffer, &jitterPacket, VOIP::FRAME_SIZE_MS, nullptr);

			if (result == JITTER_BUFFER_OK) {
				opus_decode_float(receiver.value->decoder, (uint8_t *)jitterPacket.data, jitterPacket.len, decodeBuffer, VOIP::FRAME_SIZE, 0);
				receiver.value->stream->add_buffer(decodeBuffer, VOIP::FRAME_SIZE * sizeof(float));
				memfree(jitterPacket.data);
			} else if (result == JITTER_BUFFER_MISSING) {
				print_line("[VOIP] Missing packet");
				opus_decode_float(receiver.value->decoder, nullptr, 0, decodeBuffer, VOIP::FRAME_SIZE, 0);
				receiver.value->stream->add_buffer(decodeBuffer, VOIP::FRAME_SIZE * sizeof(float));
			} else if (result == JITTER_BUFFER_INSERTION) {
				// Insert silence
				print_line("[VOIP] Insertion");
				receiver.value->stream->add_buffer(silence, VOIP::FRAME_SIZE * sizeof(float));
			} else {
				print_line("[VOIP] Unknown jitter buffer result");
			}

		}
		audioReceiversMutex.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(VOIP::FRAME_SIZE_MS / 4));
	}
}

void VOIP::init() {
	if (AudioDriver::get_singleton()->get_mix_rate() != SAMPLE_RATE) {
		print_error("[VOIP] Audio driver mix rate must be 48000Hz");
		return;
	}

	for (int i = 0; i < VOIP::FRAME_SIZE; i++) {
		silence[i] = 0.0f;
	}

	int error;
	encoder = opus_encoder_create(SAMPLE_RATE, 1, OPUS_APPLICATION_VOIP, &error);
	if (error != OPUS_OK) {
		print_error("[VOIP] Failed to create Opus encoder: " + String::num(error));
		return;
	}

	sendThread.start(SendThread, nullptr);
	decoderThread.start(DecoderThread, nullptr);
}

void VOIP::send(const AudioFrame *data, uint32_t size) {
	sendBufferMutex.lock();
	for (uint32_t i = 0; i < size; i++) {
		float sample = (data[i].l + data[i].r) / 2.0f;
		sendBuffer.push(sample);
	}
	sendBufferMutex.unlock();
}

void VOIP::receive(uint16_t clientId, uint32_t packetId, uint64_t timestamp, const void *data, uint32_t size) {
	audioReceiversMutex.lock();
	if (!audioReceivers.has(clientId)) {
		print_line(String("[VOIP] No audio receiver registered for client ") + String::num(clientId));
		audioReceiversMutex.unlock();
		return;
	}

	JitterBufferPacket jitterPacket{};
	jitterPacket.sequence = packetId;
	jitterPacket.timestamp = timestamp;
	jitterPacket.span = VOIP::FRAME_SIZE_MS;
	jitterPacket.len = size;

	void *dataCopy = memalloc(size);
	memcpy(dataCopy, data, size);
	jitterPacket.data = (char *)dataCopy;

	AudioReceiver *receiver = audioReceivers[clientId];
	jitter_buffer_put(receiver->jitterBuffer, &jitterPacket);

	audioReceiversMutex.unlock();
}

void JitterBufferDestroyCallback(void *data) {
	memfree(data);
}

void VOIP::RegisterAudioReceiver(uint32_t clientId, AudioStreamPlaybackNetReceive *stream) {
	audioReceiversMutex.lock();
	if (audioReceivers.has(clientId)) {
		print_error(String("[VOIP] Audio receiver already registered for client") + String::num(clientId));
		return;
	}

	JitterBuffer *jitterBuffer = jitter_buffer_init(VOIP::FRAME_SIZE_MS);
	jitter_buffer_ctl(jitterBuffer, JITTER_BUFFER_SET_DESTROY_CALLBACK, JitterBufferDestroyCallback);

	int error;
	OpusDecoder *decoder = opus_decoder_create(SAMPLE_RATE, 1, &error);
	if (error != OPUS_OK) {
		print_error("[VOIP] Failed to create Opus decoder: " + String::num(error));
		return;
	}

	AudioReceiver *receiver = new AudioReceiver{ stream, jitterBuffer, decoder };
	audioReceivers[clientId] = receiver;

	audioReceiversMutex.unlock();
}

void VOIP::UnregisterAudioReceiver(uint32_t clientId) {
	audioReceiversMutex.lock();
	if (!audioReceivers.has(clientId)) {
		print_error(String("[VOIP] No audio receiver registered for client") + String::num(clientId));
		return;
	}

	jitter_buffer_destroy(audioReceivers[clientId]->jitterBuffer);
	opus_decoder_destroy(audioReceivers[clientId]->decoder);
	delete audioReceivers[clientId];
	audioReceivers.erase(clientId);

	audioReceiversMutex.unlock();
}
