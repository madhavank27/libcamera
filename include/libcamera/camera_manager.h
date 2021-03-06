/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2018, Google Inc.
 *
 * camera_manager.h - Camera management
 */
#ifndef __LIBCAMERA_CAMERA_MANAGER_H__
#define __LIBCAMERA_CAMERA_MANAGER_H__

#include <memory>
#include <string>
#include <vector>

#include <libcamera/object.h>

namespace libcamera {

class Camera;
class DeviceEnumerator;
class EventDispatcher;
class PipelineHandler;

class CameraManager : public Object
{
public:
	CameraManager();
	CameraManager(const CameraManager &) = delete;
	CameraManager &operator=(const CameraManager &) = delete;
	~CameraManager();

	int start();
	void stop();

	const std::vector<std::shared_ptr<Camera>> &cameras() const { return cameras_; }
	std::shared_ptr<Camera> get(const std::string &name);

	void addCamera(std::shared_ptr<Camera> camera);
	void removeCamera(Camera *camera);

	static const std::string &version() { return version_; }

	void setEventDispatcher(std::unique_ptr<EventDispatcher> dispatcher);
	EventDispatcher *eventDispatcher();

private:
	std::unique_ptr<DeviceEnumerator> enumerator_;
	std::vector<std::shared_ptr<PipelineHandler>> pipes_;
	std::vector<std::shared_ptr<Camera>> cameras_;

	static const std::string version_;
	static CameraManager *self_;
};

} /* namespace libcamera */

#endif /* __LIBCAMERA_CAMERA_MANAGER_H__ */
