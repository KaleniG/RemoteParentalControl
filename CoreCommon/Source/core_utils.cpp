#include <sstream>

#include "core_utils.h"

namespace rpc
{
#if defined(PLATFORM_WINDOWS)
  std::string HRESULTToString(HRESULT hr)
  {
    std::ostringstream oss;
    oss << "0x" << std::hex << hr;
    return oss.str();
  }
#endif
}