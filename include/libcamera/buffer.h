/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * buffer.h - Buffer handling
 */
#ifndef __LIBCAMERA_BUFFER_H__
#define __LIBCAMERA_BUFFER_H__

#include <stdint.h>
#include <vector>

#include <libcamera/file_descriptor.h>

namespace libcamera {

class Request;

class BufferPool;
class Request;
class Stream;

class Plane final
{
public:
        Plane();
        ~Plane();

        int dmabuf() const { return fd_; } 
        int setDmabuf(int fd, unsigned int length);

        void *mem(); 
        unsigned int length() const { return length_; }

private:
        friend class Stream;

        int mmap();
        int munmap();

        int fd_; 
        unsigned int length_;
        void *mem_;
};

class BufferMemory final
{
public:
        const std::vector<Plane> &planes() const { return planes_; }
        std::vector<Plane> &planes() { return planes_; }

private:
        std::vector<Plane> planes_;
};

class BufferPool final
{
public:
        ~BufferPool();

        void createBuffers(unsigned int count);
        void destroyBuffers();

        unsigned int count() const { return buffers_.size(); }
        std::vector<BufferMemory> &buffers() { return buffers_; }

private:
        std::vector<BufferMemory> buffers_;
};


struct FrameMetadata {
	enum Status {
		FrameSuccess,
		FrameError,
		FrameCancelled,
	};

	struct Plane {
		unsigned int bytesused;
	};

	Status status;
	unsigned int sequence;
	uint64_t timestamp;
	std::vector<Plane> planes;
};

class FrameBuffer final
{
public:
	struct Plane {
		FileDescriptor fd;
		unsigned int length;
	};

	enum Status {
                BufferSuccess,
                BufferError,
                BufferCancelled,
        };


	FrameBuffer(const std::vector<Plane> &planes, unsigned int cookie = 0);

	FrameBuffer(const FrameBuffer &) = delete;
	FrameBuffer(FrameBuffer &&) = delete;

	FrameBuffer(unsigned int index = -1, const FrameBuffer *metadata = nullptr);

	FrameBuffer &operator=(const FrameBuffer &) = delete;
	FrameBuffer &operator=(FrameBuffer &&) = delete;

	const std::vector<Plane> &planes() const { return planes_; }

	Status status() const { return status_; }
	Request *request() const { return request_; }
	const FrameMetadata &metadata() const { return metadata_; };
	Stream *stream() const { return stream_; }

	unsigned int cookie() const { return cookie_; }
	void setCookie(unsigned int cookie) { cookie_ = cookie; }

	unsigned int index() const { return index_; }
	unsigned int bytesused() const { return bytesused_; }
        uint64_t timestamp() const { return timestamp_; }
        unsigned int sequence() const { return sequence_; }

	const std::array<int, 3> &dmabufs() const { return dmabuf_; }
        BufferMemory *mem() { return mem_; }

	Stream *stream_;
        unsigned int index_;
	std::array<int, 3> dmabuf_;

private:
	friend class Request; /* Needed to update request_. */
	friend class V4L2VideoDevice; /* Needed to update metadata_. */

	void cancel();

	std::vector<Plane> planes_;

	Request *request_;
	FrameMetadata metadata_;

	unsigned int cookie_;

        BufferMemory *mem_;

	Status status_;
	unsigned int bytesused_;
        uint64_t timestamp_;
        unsigned int sequence_;
};

} /* namespace libcamera */

#endif /* __LIBCAMERA_BUFFER_H__ */
