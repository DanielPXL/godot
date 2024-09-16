#ifndef AUDIO_STREAM_NETRECEIVE_H
#define AUDIO_STREAM_NETRECEIVE_H

#include "servers/audio/audio_stream.h"
#include "core/os/mutex.h"

class AudioStreamPlaybackNetReceive;

class AudioStreamNetReceive : public AudioStream {
	GDCLASS(AudioStreamNetReceive, AudioStream);
	friend class AudioStreamPlaybackNetReceive;

private:
	uint32_t user_id;

protected:
	static void _bind_methods();

public:
	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual String get_stream_name() const override;

	virtual double get_length() const override;

	virtual bool is_monophonic() const override;

	uint32_t get_user_id() const;
	void set_user_id(uint32_t user_id);

	AudioStreamNetReceive();
};

class AudioStreamPlaybackNetReceive : public AudioStreamPlaybackResampled {
	GDCLASS(AudioStreamPlaybackNetReceive, AudioStreamPlaybackResampled);
	friend class AudioStreamNetReceive;

private:
	Ref<AudioStreamNetReceive> m_stream;
	bool m_active;

	AudioFrame* m_buffer;
	uint32_t m_bufferSize;
	uint32_t m_readPos;
	uint32_t m_writePos;

	Mutex m_bufferMutex;

protected:
	virtual int _mix_internal(AudioFrame *p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;
	virtual double get_playback_position() const override;

public:
	virtual int mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames) override;

	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override;

	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	void add_frames(const AudioFrame* p_frames, uint32_t p_frameCount);
	void add_buffer(const void* p_data, uint32_t size);
	uint32_t frames_available_write() const;
	uint32_t frames_available_read() const;
	uint32_t get_size() const;

	AudioStreamPlaybackNetReceive();
	~AudioStreamPlaybackNetReceive();
};

#endif // AUDIO_STREAM_NETRECEIVE_H
