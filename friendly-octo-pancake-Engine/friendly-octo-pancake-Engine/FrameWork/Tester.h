#pragma once

#include "Logger_Import/loguru.hpp"
#include "EventSystem/EventSystem.h"
#include "Singletons/baseSingleton.h"
#include "Singletons/baseSingletonStorrage.h"
#include "Singletons/baseSingletonFactory.h"
#include <thread>
#include <stdexcept>

namespace FrameWork {
	void logguruTest(int argc, char* argv[]);

	void singletonTest();

	bool testFramework(int argc, char* argv[]) {

		//logguruTest(argc, argv); is ok

		singletonTest();

		return true;
	}

	void logguruTest(int argc, char* argv[]) {
		// Optional, but useful to time-stamp the start of the log.
		// Will also detect verbosity level on command line as -v.
		FrameWork_Logger_loguru::init(argc, argv);

		// Put every log message in "everything.log":
		FrameWork_Logger_loguru::add_file("everything.log", FrameWork_Logger_loguru::Append, FrameWork_Logger_loguru::Verbosity_MAX);

		// Only log INFO, WARNING, ERROR and FATAL to "latest_readable.log":
		FrameWork_Logger_loguru::add_file("latest_readable.log", FrameWork_Logger_loguru::Truncate, FrameWork_Logger_loguru::Verbosity_INFO);

		// Only show most relevant things on stderr:
		FrameWork_Logger_loguru::g_stderr_verbosity = 6;

		LOG_SCOPE_F(INFO, "Will indent all log messages within this scope.");
		LOG_F(INFO, "I'm hungry for some %.3f!", 3.14159);
		LOG_F(3, "Will only show if verbosity is 2 or higher");
		VLOG_F(0, "Use vlog for dynamic log level (integer in the range 0-9, inclusive)");
		LOG_IF_F(ERROR, false, "Will not show");
		LOG_IF_F(ERROR, true, "Will show");
		
		CHECK_GT_F(20, 0); // Will print the value of `length` on failure.

		// Each function also comes with a version prefixed with D for Debug:
		DLOG_F(INFO, "Only written in debug-builds");

		// Turn off writing to stderr:
		FrameWork_Logger_loguru::g_stderr_verbosity = FrameWork_Logger_loguru::Verbosity_OFF;

		// Turn off writing err/warn in red:
		FrameWork_Logger_loguru::g_colorlogtostderr = false;

		// Throw exceptions instead of aborting on CHECK fails:
		FrameWork_Logger_loguru::set_fatal_handler([](const FrameWork_Logger_loguru::Message& message) {
			throw std::runtime_error(std::string(message.prefix) + message.message);
			});
	}

	class testSingletonMapStorrage : public FrameWork_Singletons::baseMapStorrage<int, std::string, testSingletonMapStorrage> {

	};

	void singletonTest() {
		auto test = testSingletonMapStorrage::getInstance();
		std::string s0 = "test";
		testSingletonMapStorrage::getInstance()->addOnId(0, &s0);
		 testSingletonMapStorrage::getInstance()->getById(0);
	}
}
FrameWork::testSingletonMapStorrage* FrameWork::testSingletonMapStorrage::instance = 0;