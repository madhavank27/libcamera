/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * event-dispatcher.cpp - Event dispatcher test
 */

#include <iostream>
#include <signal.h>
#include <sys/time.h>

#include <libcamera/camera_manager.h>
#include <libcamera/event_dispatcher.h>
#include <libcamera/timer.h>

#include "test.h"

using namespace std;
using namespace libcamera;

class EventDispatcherTest : public Test
{
protected:
	static void sigAlarmHandler(int)
	{
		cout << "SIGALARM received" << endl;
	}

	int init()
	{
		struct sigaction sa = {};
		sa.sa_handler = &sigAlarmHandler;
		sigaction(SIGALRM, &sa, nullptr);

		return 0;
	}

	int run()
	{
		EventDispatcher *dispatcher = CameraManager::instance()->eventDispatcher();
		Timer timer;

		/* Event processing interruption by signal. */
		struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);

		timer.start(1000);

		struct itimerval itimer = {};
		itimer.it_value.tv_usec = 500000;
		setitimer(ITIMER_REAL, &itimer, nullptr);

		dispatcher->processEvents();

		struct timespec stop;
		clock_gettime(CLOCK_MONOTONIC, &stop);
		int duration = (stop.tv_sec - start.tv_sec) * 1000;
		duration += (stop.tv_nsec - start.tv_nsec) / 1000000;

		if (abs(duration - 1000) > 50) {
			cout << "Event processing restart test failed" << endl;
			return TestFail;
		}

		return TestPass;
	}

	void cleanup()
	{
	}
};

TEST_REGISTER(EventDispatcherTest)