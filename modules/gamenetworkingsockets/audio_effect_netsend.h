#ifndef AUDIO_EFFECT_NETSEND_H
#define AUDIO_EFFECT_NETSEND_H

#include "servers/audio/audio_effect.h"

class AudioEffectNetSend;

class AudioEffectNetSendInstance : public AudioEffectInstance {
	GDCLASS(AudioEffectNetSendInstance, AudioEffectInstance);
	friend class AudioEffectNetSend;
	Ref<AudioEffectNetSend> base;

	uint32_t m_voipPacketId = 0;
	void *m_buffer = nullptr;
	uint32_t m_bufferSize = 0;

public:
	virtual void process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) override;
};

class AudioEffectNetSend : public AudioEffect {
	GDCLASS(AudioEffectNetSend, AudioEffect);

	friend class AudioEffectNetSendInstance;
	bool enabled;

protected:
	static void _bind_methods();

public:
	AudioEffectNetSend();
	Ref<AudioEffectInstance> instantiate() override;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;
};

#endif // AUDIO_EFFECT_NETSEND_H
