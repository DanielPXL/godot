#include "audio_effect_netsend.h"
#include "GodotNetworkingSockets.h"
#include "voip.h"

void AudioEffectNetSendInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {
	if (base->enabled) {
		VOIP::send(p_src_frames, p_frame_count);
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
