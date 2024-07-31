#include "voip.h"
#include "circular_buffer.h"
#include <chrono>
#include <thread>

Thread sendThread;
CircularBuffer<float> sendBuffer = CircularBuffer<float>(VOIP::SEND_BUFFER_SIZE);
Mutex sendBufferMutex;
float *workingBuffer = new float[VOIP::FRAME_SIZE];
void *packetBuffer = new uint8_t[VOIP::PACKET_SIZE];
uint32_t voipPacketId = 0;

void SendThread(void *args) {
	while (true) {
		sendBufferMutex.lock();
		while (sendBuffer.health() >= VOIP::FRAME_SIZE) {
			for (int i = 0; i < VOIP::FRAME_SIZE; i++) {
				workingBuffer[i] = sendBuffer.pop();
			}
			sendBufferMutex.unlock();

			// TODO: Opus encode

			*(uint16_t *)((uint8_t *)packetBuffer + 0) = 1; // Packet id for voip data
			*(uint32_t *)((uint8_t *)packetBuffer + sizeof(uint16_t)) = 0; // User ID (filled in by server)
			*(uint32_t *)((uint8_t *)packetBuffer + sizeof(uint16_t) + sizeof(uint32_t)) = voipPacketId;
			voipPacketId++;

			void* data = (uint8_t *)packetBuffer + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);
			memcpy(data, workingBuffer, VOIP::FRAME_SIZE * sizeof(float));
			GodotNetClient::SendRaw(packetBuffer, VOIP::PACKET_SIZE, k_nSteamNetworkingSend_UnreliableNoDelay, 0);

			sendBufferMutex.lock();
		}
		sendBufferMutex.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void VOIP::init() {
	if (AudioDriver::get_singleton()->get_mix_rate() != SAMPLE_RATE) {
		print_error("[VOIP] Audio driver mix rate must be 48000Hz");
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
