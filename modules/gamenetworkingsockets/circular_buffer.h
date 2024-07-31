#pragma once
#include "core/object/ref_counted.h"

template <typename T>
class CircularBuffer {
public:
	CircularBuffer(size_t size);
	~CircularBuffer();

	inline void push(T value);
	inline T pop();

	inline size_t health();
	inline size_t remaining_capacity();
	inline size_t size();

private:
	T *m_buffer;
	size_t m_size;
	size_t m_readIndex;
	size_t m_writeIndex;
};

template <typename T>
CircularBuffer<T>::CircularBuffer(size_t size) {
	m_buffer = memnew_arr(T, size);
	m_size = size;
	m_readIndex = 0;
	m_writeIndex = 0;
}

template <typename T>
CircularBuffer<T>::~CircularBuffer() {
	memdelete_arr(m_buffer);
}

template <typename T>
void CircularBuffer<T>::push(T value) {
	m_buffer[m_writeIndex] = value;
	m_writeIndex = (m_writeIndex + 1) % m_size;
	if (m_writeIndex == m_readIndex) {
		m_readIndex = (m_readIndex + 1) % m_size;
#if DEV_ENABLED
		print_error("CircularBuffer overflowed");
#endif
	}
}

template <typename T>
T CircularBuffer<T>::pop() {
	if (m_readIndex == m_writeIndex) {
		return T();
	}
	T value = m_buffer[m_readIndex];
	m_readIndex = (m_readIndex + 1) % m_size;
	return value;
}

template <typename T>
size_t CircularBuffer<T>::health() {
	return (m_writeIndex - m_readIndex + m_size) % m_size;
}

template <typename T>
size_t CircularBuffer<T>::remaining_capacity() {
	return m_size - health() - 1;
}

template <typename T>
size_t CircularBuffer<T>::size() {
	return m_size;
}
