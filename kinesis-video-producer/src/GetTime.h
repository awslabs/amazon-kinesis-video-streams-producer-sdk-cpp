#include <chrono>

#ifndef __GET_TIME_DEFINITIONS__
#define __GET_TIME_DEFINITIONS__

#pragma once

namespace com { namespace amazonaws { namespace kinesis { namespace video {

std::chrono::time_point<std::chrono::system_clock> systemCurrentTime();


} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
#endif  /* __TIME_DEFINITIONS__ */
