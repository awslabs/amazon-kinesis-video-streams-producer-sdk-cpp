#include "GetTime.h"

namespace com { namespace amazonaws { namespace kinesis { namespace video {
std::chrono::time_point<std::chrono::system_clock> systemCurrentTime() {
  return std::chrono::system_clock::now();
  // if you local time has 10 minutes drift
  //return std::chrono::system_clock::now() + std::chrono::microseconds(std::chrono::minutes(10));
}

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
