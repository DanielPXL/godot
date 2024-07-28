#include "audio_effect_netsend.h"
#include "GodotNetworkingSockets.h"

void AudioEffectNetSendInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {
	if (base->enabled) {
		uint32_t packetSize = sizeof(uint16_t) + 2 * sizeof(uint32_t) + p_frame_count * sizeof(AudioFrame);
		if (m_bufferSize < packetSize) {
			if (m_buffer) {
				memfree(m_buffer);
			}
			m_buffer = memalloc(packetSize);
			m_bufferSize = packetSize;
		}

		*(uint16_t *)((uint8_t *)m_buffer + 0) = 1; // VOIP packet ID
		*(uint32_t *)((uint8_t *)m_buffer + sizeof(uint16_t)) = 0; // User ID (filled in by server)
		*(uint32_t *)((uint8_t *)m_buffer + sizeof(uint16_t) + sizeof(uint32_t)) = m_voipPacketId;
		m_voipPacketId++;

		memcpy((uint8_t*)m_buffer + sizeof(uint16_t) + 2 * sizeof(uint32_t), p_src_frames, p_frame_count * sizeof(AudioFrame));

		GodotNetClient::SendRaw(m_buffer, packetSize, k_nSteamNetworkingSend_UnreliableNoDelay, 0);
	}

	// Pass through
	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i];
	}
}

Ref<AudioEffectInstance> AudioEffectNetSend::instantiate() {
	Ref<AudioEffectNetSendInstance> ins;
	ins.instantiate();
	ins->base = Ref<AudioEffectNetSend>(this);

	return ins;
}

void AudioEffectNetSend::set_enabled(bool p_enabled) {
	enabled = p_enabled;
}

bool AudioEffectNetSend::is_enabled() const {
	return enabled;
}

void AudioEffectNetSend::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &AudioEffectNetSend::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &AudioEffectNetSend::is_enabled);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
}

AudioEffectNetSend::AudioEffectNetSend() {
	enabled = false;
}
