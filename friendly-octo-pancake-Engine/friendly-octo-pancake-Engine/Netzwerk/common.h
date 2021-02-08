#ifndef _COMMON_
#define _COMMON_

#include "../FrameWork/Logger_Import/loguru.hpp"

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include "include/asio.hpp"
#include "include/asio/ts/buffer.hpp"
#include "include/asio/ts/internet.hpp"


#endif