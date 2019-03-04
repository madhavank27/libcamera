/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * libcamera V4L2 device format handling test
 */

#include <climits>
#include <iostream>

#include "v4l2_device.h"

#include "v4l2_device_test.h"

using namespace std;
using namespace libcamera;

class Format : public V4L2DeviceTest
{
protected:
	int run() override;
};

int Format::run()
{
	V4L2DeviceFormat format = {};

	int ret = capture_->getFormat(&format);
	if (ret) {
		cerr << "Failed to get format" << endl;
		return TestFail;
	}

	format.width = UINT_MAX;
	format.height = UINT_MAX;
	ret = capture_->setFormat(&format);
	if (ret) {
		cerr << "Failed to set format: image resolution is invalid: "
		     << "(UINT_MAX x UINT_MAX) but setFormat() should not fail."
		     << endl;
		return TestFail;
	}

	if (format.width == UINT_MAX || format.height == UINT_MAX) {
		cerr << "Failed to update image format = (UINT_MAX x UINT_MAX)"
		     << endl;
		return TestFail;
	}

	return TestPass;
}

TEST_REGISTER(Format);