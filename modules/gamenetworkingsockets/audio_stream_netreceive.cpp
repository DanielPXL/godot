#include "audio_stream_netreceive.h"
#include "GodotNetworkingSockets.h"

AudioStreamNetReceive::AudioStreamNetReceive() {
	user_id = 0;
}

void AudioStreamNetReceive::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_user_id"), &AudioStreamNetReceive::get_user_id);
	ClassDB::bind_method(D_METHOD("set_user_id", "user_id"), &AudioStreamNetReceive::set_user_id);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "user_id"), "set_user_id", "get_user_id");
}

Ref<AudioStreamPlayback> AudioStreamNetReceive::instantiate_playback() {
	Ref<AudioStreamPlaybackNetReceive> playback;
	playback.instantiate();

	playback->m_stream = Ref<AudioStreamNetReceive>(this);

	return playback;
}

String AudioStreamNetReceive::get_stream_name() const {
	return "NetReceive";
}

double AudioStreamNetReceive::get_length() const {
	return 0;
}

bool AudioStreamNetReceive::is_monophonic() const {
	return true;
}

uint16_t AudioStreamNetReceive::get_user_id() const {
	return user_id;
}

void AudioStreamNetReceive::set_user_id(uint16_t p_user_id) {
	user_id = p_user_id;
}

AudioStreamPlaybackNetReceive::AudioStreamPlaybackNetReceive() {
	m_active = false;
	m_bufferSize = 44100 * 2000 / 1000; // 2000ms buffer
	m_buffer = memnew_arr(AudioFrame, m_bufferSize);
	m_readPos = 0;
	m_writePos = 0;
}

AudioStreamPlaybackNetReceive::~AudioStreamPlaybackNetReceive() {
	memdelete_arr(m_buffer);
}

int AudioStreamPlaybackNetReceive::_mix_internal(AudioFrame* p_buffer, int p_frames) {
	static bool start = false;
	if (frames_available_write() < m_bufferSize / 2) {
		start = true;
	}

	int frames = 0;
	if (start) {
		m_bufferMutex.lock();
		while (frames < p_frames) {
			if (m_readPos == m_writePos) {
				break;
			}

			p_buffer[frames] = m_buffer[m_readPos];
			m_readPos = (m_readPos + 1) % m_bufferSize;
			frames++;
		}
		m_bufferMutex.unlock();
	}

	// Fill the rest of the buffer with silence
	for (int i = frames; i < p_frames; i++) {
		p_buffer[i].l = 0;
		p_buffer[i].r = 0;
	}

	return p_frames;
}

float AudioStreamPlaybackNetReceive::get_stream_sampling_rate() {
	return 44100;
}

double AudioStreamPlaybackNetReceive::get_playback_position() const {
	return 0;
}

int AudioStreamPlaybackNetReceive::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) {
	return AudioStreamPlaybackResampled::mix(p_buffer, p_rate_scale, p_frames);
}

void AudioStreamPlaybackNetReceive::start(double p_from_pos) {
	if (m_active) {
		return;
	}

	GodotNetClient::RegisterAudioReceiver(m_stream->user_id, this);
	m_active = true;
}

void AudioStreamPlaybackNetReceive::stop() {
	if (m_active) {
		GodotNetClient::UnregisterAudioReceiver(m_stream->user_id);
		m_active = false;
	}
}

bool AudioStreamPlaybackNetReceive::is_playing() const {
	return true;
}

int AudioStreamPlaybackNetReceive::get_loop_count() const {
	return 0;
}

void AudioStreamPlaybackNetReceive::seek(double p_time) {

}

void AudioStreamPlaybackNetReceive::tag_used_streams() {
	m_stream->tag_used(0);
}

void AudioStreamPlaybackNetReceive::add_frames(const AudioFrame* p_frames, uint32_t p_frameCount) {
	m_bufferMutex.lock();
	for (uint32_t i = 0; i < p_frameCount; i++) {
		m_buffer[m_writePos] = p_frames[i];
		m_writePos = (m_writePos + 1) % m_bufferSize;

		if (m_writePos == m_readPos) {
			m_writePos = (m_writePos - 1 + m_bufferSize) % m_bufferSize;
			break;
		}
	}
	m_bufferMutex.unlock();
}

void AudioStreamPlaybackNetReceive::add_buffer(const void* p_buffer, uint32_t p_bufferSize) {
	const AudioFrame* frames = (const AudioFrame*)p_buffer;
	uint32_t frameCount = p_bufferSize / sizeof(AudioFrame);
	add_frames(frames, frameCount);
}

uint32_t AudioStreamPlaybackNetReceive::frames_available_write() const {
	return (m_readPos - m_writePos - 1) % m_bufferSize;
}
