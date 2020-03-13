/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * stream.cpp - Video stream for a Camera
 */

#include <libcamera/stream.h>

#include <algorithm>
#include <array>
#include <iomanip>
#include <limits.h>
#include <sstream>

#include <libcamera/request.h>

#include "log.h"
#include "utils.h"

/**
 * \file stream.h
 * \brief Video stream for a Camera
 *
 * A camera device can provide frames in different resolutions and formats
 * concurrently from a single image source. The Stream class represents
 * one of the multiple concurrent streams.
 *
 * All streams exposed by a camera device share the same image source and are
 * thus not fully independent. Parameters related to the image source, such as
 * the exposure time or flash control, are common to all streams. Other
 * parameters, such as format or resolution, may be specified per-stream,
 * depending on the capabilities of the camera device.
 *
 * Camera devices expose at least one stream, and may expose additional streams
 * based on the device capabilities. This can be used, for instance, to
 * implement concurrent viewfinder and video capture, or concurrent viewfinder,
 * video capture and still image capture.
 */

namespace libcamera {

LOG_DEFINE_CATEGORY(Stream)

/**
 * \class StreamFormats
 * \brief Hold information about supported stream formats
 *
 * The StreamFormats class holds information about the pixel formats and frame
 * sizes a stream supports. The class groups size information by the pixel
 * format which can produce it.
 *
 * There are two ways to examine the size information, as a range or as a list
 * of discrete sizes. When sizes are viewed as a range it describes the minimum
 * and maximum width and height values. The range description can include
 * horizontal and vertical steps.
 *
 * When sizes are viewed as a list of discrete sizes they describe the exact
 * dimensions which can be selected and used.
 *
 * Pipeline handlers can create StreamFormats describing each pixel format using
 * either a range or a list of discrete sizes. The StreamFormats class attempts
 * to translate between the two different ways to view them. The translations
 * are performed as:
 *
 *  - If the StreamFormat is constructed using a list of discrete image sizes
 *    and a range is requested, it gets created by taking the minimum and
 *    maximum width/height in the list. The step information is not recreated
 *    and is set to 0 to indicate the range is generated.
 *
 *  - If the image sizes used to construct a StreamFormat are expressed as a
 *    range and a list of discrete sizes is requested, one which fits inside
 *    that range are selected from a list of common sizes. The step information
 *    is taken into consideration when generating the sizes.
 *
 * Applications examining sizes as a range with step values of 0 shall be
 * aware that the range are generated from a list of discrete sizes and there
 * could be a large number of possible Size combinations that may not be
 * supported by the Stream.
 *
 * All sizes retrieved from StreamFormats shall be treated as advisory and no
 * size shall be considered to be supported until it has been verified using
 * CameraConfiguration::validate().
 *
 * \todo Review the usage patterns of this class, and cache the computed
 * pixelformats(), sizes() and range() if this would improve performances.
 */

StreamFormats::StreamFormats()
{
}

/**
 * \brief Construct a StreamFormats object with a map of image formats
 * \param[in] formats A map of pixel formats to a sizes description
 */
StreamFormats::StreamFormats(const std::map<PixelFormat, std::vector<SizeRange>> &formats)
	: formats_(formats)
{
}

/**
 * \brief Retrieve the list of supported pixel formats
 * \return The list of supported pixel formats
 */
std::vector<PixelFormat> StreamFormats::pixelformats() const
{
	std::vector<PixelFormat> formats;

	for (auto const &it : formats_)
		formats.push_back(it.first);

	return formats;
}

/**
 * \brief Retrieve the list of frame sizes supported for \a pixelformat
 * \param[in] pixelformat PixelFormat to retrieve sizes for
 *
 * If the sizes described for \a pixelformat are discrete they are returned
 * directly.
 *
 * If the sizes are described as a range, a list of discrete sizes are computed
 * from a list of common resolutions that fit inside the described range. When
 * computing the discrete list step values are considered but there are no
 * guarantees that all sizes computed are supported.
 *
 * \return A list of frame sizes or an empty list on error
 */
std::vector<Size> StreamFormats::sizes(PixelFormat pixelformat) const
{
	/*
	 * Sizes to try and extract from ranges.
	 * \todo Verify list of resolutions are good, current list compiled
	 * from v4l2 documentation and source code as well as lists of
	 * common frame sizes.
	 */
	static const std::array<Size, 53> rangeDiscreteSizes = {
		Size(160, 120),
		Size(240, 160),
		Size(320, 240),
		Size(400, 240),
		Size(480, 320),
		Size(640, 360),
		Size(640, 480),
		Size(720, 480),
		Size(720, 576),
		Size(768, 480),
		Size(800, 600),
		Size(854, 480),
		Size(960, 540),
		Size(960, 640),
		Size(1024, 576),
		Size(1024, 600),
		Size(1024, 768),
		Size(1152, 864),
		Size(1280, 1024),
		Size(1280, 1080),
		Size(1280, 720),
		Size(1280, 800),
		Size(1360, 768),
		Size(1366, 768),
		Size(1400, 1050),
		Size(1440, 900),
		Size(1536, 864),
		Size(1600, 1200),
		Size(1600, 900),
		Size(1680, 1050),
		Size(1920, 1080),
		Size(1920, 1200),
		Size(2048, 1080),
		Size(2048, 1152),
		Size(2048, 1536),
		Size(2160, 1080),
		Size(2560, 1080),
		Size(2560, 1440),
		Size(2560, 1600),
		Size(2560, 2048),
		Size(2960, 1440),
		Size(3200, 1800),
		Size(3200, 2048),
		Size(3200, 2400),
		Size(3440, 1440),
		Size(3840, 1080),
		Size(3840, 1600),
		Size(3840, 2160),
		Size(3840, 2400),
		Size(4096, 2160),
		Size(5120, 2160),
		Size(5120, 2880),
		Size(7680, 4320),
	};
	std::vector<Size> sizes;

	/* Make sure pixel format exists. */
	auto const &it = formats_.find(pixelformat);
	if (it == formats_.end())
		return {};

	/* Try creating a list of discrete sizes. */
	const std::vector<SizeRange> &ranges = it->second;
	bool discrete = true;
	for (const SizeRange &range : ranges) {
		if (range.min != range.max) {
			discrete = false;
			break;
		}
		sizes.emplace_back(range.min);
	}

	/* If discrete not possible generate from range. */
	if (!discrete) {
		if (ranges.size() != 1) {
			LOG(Stream, Error) << "Range format is ambiguous";
			return {};
		}

		const SizeRange &limit = ranges.front();
		sizes.clear();

		for (const Size &size : rangeDiscreteSizes)
			if (limit.contains(size))
				sizes.push_back(size);
	}

	std::sort(sizes.begin(), sizes.end());

	return sizes;
}

/**
 * \brief Retrieve the range of minimum and maximum sizes
 * \param[in] pixelformat PixelFormat to retrieve range for
 *
 * If the size described for \a pixelformat is a range, that range is returned
 * directly. If the sizes described are a list of discrete sizes, a range is
 * created from the minimum and maximum sizes in the list. The step values of
 * the range are set to 0 to indicate that the range is generated and that not
 * all image sizes contained in the range might be supported.
 *
 * \return A range of valid image sizes or an empty range on error
 */
SizeRange StreamFormats::range(PixelFormat pixelformat) const
{
	auto const it = formats_.find(pixelformat);
	if (it == formats_.end())
		return {};

	const std::vector<SizeRange> &ranges = it->second;
	if (ranges.size() == 1)
		return ranges[0];

	LOG(Stream, Debug) << "Building range from discrete sizes";
	SizeRange range(UINT_MAX, UINT_MAX, 0, 0);
	for (const SizeRange &limit : ranges) {
		if (limit.min < range.min)
			range.min = limit.min;

		if (limit.max > range.max)
			range.max = limit.max;
	}

	range.hStep = 0;
	range.vStep = 0;

	return range;
}

/**
 * \struct StreamConfiguration
 * \brief Configuration parameters for a stream
 *
 * The StreamConfiguration structure models all information which can be
 * configured for a single video stream.
 */

/**
 * \todo This method is deprecated and should be removed once all pipeline
 * handlers provied StreamFormats.
 */
StreamConfiguration::StreamConfiguration()
	: pixelFormat(0), memoryType(InternalMemory), stream_(nullptr)
{
}

/**
 * \brief Construct a configuration with stream formats
 */
StreamConfiguration::StreamConfiguration(const StreamFormats &formats)
	: pixelFormat(0), memoryType(InternalMemory), stream_(nullptr), formats_(formats)
{
}

/**
 * \var StreamConfiguration::size
 * \brief Stream size in pixels
 */

/**
 * \var StreamConfiguration::pixelFormat
 * \brief Stream pixel format
 */

/**
 * \var StreamConfiguration::bufferCount
 * \brief Requested number of buffers to allocate for the stream
 */

/**
 * \fn StreamConfiguration::stream()
 * \brief Retrieve the stream associated with the configuration
 *
 * When a camera is configured with Camera::configure() Stream instances are
 * associated with each stream configuration entry. This method retrieves the
 * associated Stream, which remains valid until the next call to
 * Camera::configure() or Camera::release().
 *
 * \return The stream associated with the configuration
 */

/**
 * \fn StreamConfiguration::setStream()
 * \brief Associate a stream with a configuration
 *
 * This method is meant for the PipelineHandler::configure() method and shall
 * not be called by applications.
 *
 * \param[in] stream The stream
 */

/**
 * \fn StreamConfiguration::formats()
 * \brief Retrieve advisory stream format information
 *
 * This method retrieves information about the pixel formats and sizes supported
 * by the stream configuration. The sizes are advisory and not all of them are
 * guaranteed to be supported by the stream. Users shall always inspect the size
 * in the stream configuration after calling CameraConfiguration::validate().
 *
 * \return Stream formats information
 */

/**
 * \brief Assemble and return a string describing the configuration
 *
 * \return A string describing the StreamConfiguration
 */
std::string StreamConfiguration::toString() const
{
	std::stringstream ss;
	ss << size.toString() << "-" << utils::hex(pixelFormat);
	return ss.str();
}

/**
 * \enum StreamRole
 * \brief Identify the role a stream is intended to play
 *
 * The StreamRole describes how an application intends to use a stream. Roles
 * are specified by applications and passed to cameras, that then select the
 * most appropriate streams and their default configurations.
 *
 * \var StillCapture
 * The stream is intended to capture high-resolution, high-quality still images
 * with low frame rate. The captured frames may be exposed with flash.
 * \var VideoRecording
 * The stream is intended to capture video for the purpose of recording or
 * streaming. The video stream may produce a high frame rate and may be
 * enhanced with video stabilization.
 * \var Viewfinder
 * The stream is intended to capture video for the purpose of display on the
 * local screen. Trade-offs between quality and usage of system resources are
 * acceptable.
 */

/**
 * \typedef StreamRoles
 * \brief A vector of StreamRole
 */

/**
 * \class Stream
 * \brief Video stream for a camera
 *
 * The Stream class models all static information which are associated with a
 * single video stream. Streams are exposed by the Camera object they belong to.
 *
 * Cameras may supply more than one stream from the same video source. In such
 * cases an application can inspect all available streams and select the ones
 * that best fit its use case.
 *
 * \todo Add capabilities to the stream API. Without this the Stream class only
 * serves to reveal how many streams of unknown capabilities a camera supports.
 * This in itself is productive as it allows applications to configure and
 * capture from one or more streams even if they won't be able to select the
 * optimal stream for the task.
 */

/**
 * \brief Construct a stream with default parameters
 */
Stream::Stream()
{
}

std::unique_ptr<FrameBuffer> Stream::createBuffer(unsigned int index)
{
        if (memoryType_ != InternalMemory) {
                LOG(Stream, Error) << "Invalid stream memory type";
                return nullptr;
        }

        if (index >= bufferPool_.count()) {
                LOG(Stream, Error) << "Invalid buffer index " << index;
                return nullptr;
        }

        FrameBuffer *buffer = new FrameBuffer();
        buffer->index_ = index;
        buffer->stream_ = this;

        return std::unique_ptr<FrameBuffer>(buffer);
}

std::unique_ptr<FrameBuffer> Stream::createBuffer(const std::array<int, 3> &fds)
{
        if (memoryType_ != ExternalMemory) {
                LOG(Stream, Error) << "Invalid stream memory type";
                return nullptr;
        }

        FrameBuffer *buffer = new FrameBuffer();
        buffer->dmabuf_ = fds;
        buffer->stream_ = this;

        return std::unique_ptr<FrameBuffer>(buffer);
}


int Stream::mapBuffer(const FrameBuffer *buffer)
{
        ASSERT(memoryType_ == ExternalMemory);

        if (bufferCache_.empty())
                return -ENOMEM;

        const std::array<int, 3> &dmabufs = buffer->dmabufs();

        /*
         * Try to find a previously mapped buffer in the cache. If we miss, use
         * the oldest entry in the cache.
         */
        auto map = std::find_if(bufferCache_.begin(), bufferCache_.end(),
                                [&](std::pair<std::array<int, 3>, unsigned int> &entry) {
                                        return entry.first == dmabufs;
                                });
        if (map == bufferCache_.end())
                map = bufferCache_.begin();

        /*
         * Update the dmabuf file descriptors of the entry. We can't assume that
         * identical file descriptor numbers refer to the same dmabuf object as
         * it may have been closed and its file descriptor reused. We thus need
         * to update the plane's internally cached mmap()ed memory.
         */
        unsigned int index = map->second;
        BufferMemory *mem = &bufferPool_.buffers()[index];
        mem->planes().clear();

        for (unsigned int i = 0; i < dmabufs.size(); ++i) {
                if (dmabufs[i] == -1)
                        break;

                mem->planes().emplace_back();
                mem->planes().back().setDmabuf(dmabufs[i], 0);
        }

        /* Remove the buffer from the cache and return its index. */
        bufferCache_.erase(map);
        return index;
}

void Stream::unmapBuffer(const FrameBuffer *buffer)
{
        ASSERT(memoryType_ == ExternalMemory);

        bufferCache_.emplace_back(buffer->dmabufs(), buffer->index());
}

void Stream::createBuffers(MemoryType memory, unsigned int count)
{
        destroyBuffers();
        if (count == 0)
                return;

        memoryType_ = memory;
        bufferPool_.createBuffers(count);

        /* Streams with internal memory usage do not need buffer mapping. */
        if (memoryType_ == InternalMemory)
                return;

        /*
         * Prepare for buffer mapping by adding all buffer memory entries to the
         * cache.
         */
        bufferCache_.clear();
        for (unsigned int i = 0; i < bufferPool_.count(); ++i)
                bufferCache_.emplace_back(std::array<int, 3>{ -1, -1, -1 }, i);
}

void Stream::destroyBuffers()
{
        bufferPool_.destroyBuffers();
}

/**
 * \fn Stream::configuration()
 * \brief Retrieve the active configuration of the stream
 * \return The active configuration of the stream
 */

/**
 * \var Stream::configuration_
 * \brief The stream configuration
 *
 * The configuration for the stream is set by any successful call to
 * Camera::configure() that includes the stream, and remains valid until the
 * next call to Camera::configure() regardless of if it includes the stream.
 */

} /* namespace libcamera */
