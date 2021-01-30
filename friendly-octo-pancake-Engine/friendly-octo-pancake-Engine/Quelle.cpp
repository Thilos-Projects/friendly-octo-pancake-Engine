#include "FrameWork/Logger_Import/loguru.hpp"
#include "FrameWork/EntryPoint.h"

int main(int argc, char* argv[])
{
	FrameWork_Logger_loguru::g_stderr_verbosity = 2;

	FrameWork::mainClass::run();
	return 0;
}