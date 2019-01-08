/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2018, Google Inc.
 *
 * vimc.cpp - Pipeline handler for the vimc device
 */

#include <libcamera/camera.h>

#include "device_enumerator.h"
#include "media_device.h"
#include "pipeline_handler.h"

namespace libcamera {

class PipeHandlerVimc : public PipelineHandler
{
public:
	PipeHandlerVimc();
	~PipeHandlerVimc();

	bool match(DeviceEnumerator *enumerator);

	unsigned int count();
	Camera *camera(unsigned int id) final;

private:
	MediaDevice *dev_;
	Camera *camera_;
};

PipeHandlerVimc::PipeHandlerVimc()
	: dev_(nullptr), camera_(nullptr)
{
}

PipeHandlerVimc::~PipeHandlerVimc()
{
	if (camera_)
		camera_->put();

	if (dev_)
		dev_->release();
}

unsigned int PipeHandlerVimc::count()
{
	return 1;
}

Camera *PipeHandlerVimc::camera(unsigned int id)
{
	if (id != 0)
		return nullptr;

	return camera_;
}

bool PipeHandlerVimc::match(DeviceEnumerator *enumerator)
{
	DeviceMatch dm("vimc");

	dm.add("Raw Capture 0");
	dm.add("Raw Capture 1");
	dm.add("RGB/YUV Capture");
	dm.add("Sensor A");
	dm.add("Sensor B");
	dm.add("Debayer A");
	dm.add("Debayer B");
	dm.add("RGB/YUV Input");
	dm.add("Scaler");

	dev_ = enumerator->search(dm);
	if (!dev_)
		return false;

	dev_->acquire();

	/*
	 * NOTE: A more complete Camera implementation could
	 * be passed the MediaDevice(s) it controls here or
	 * a reference to the PipelineHandler. Which method
	 * will be chosen depends on how the Camera
	 * object is modeled.
	 */
	camera_ = new Camera("Dummy VIMC Camera");

	return true;
}

REGISTER_PIPELINE_HANDLER(PipeHandlerVimc);

} /* namespace libcamera */