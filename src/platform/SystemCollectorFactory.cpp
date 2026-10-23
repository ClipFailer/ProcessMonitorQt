#include "core/ISystemCollector.hpp"

#if defined(_WIN32)
#include "windows/WinSystemCollector.hpp"
#elif defined(__linux__)
#include "linux/LinuxSystemCollector.hpp"
#endif

namespace pm::core {

std::unique_ptr<ISystemCollector> createSystemCollector() {
#if defined(_WIN32)
    return std::make_unique<pm::platform::win::WinSystemCollector>();
#elif defined(__linux__)
    return std::make_unique<pm::platform::linux_os::LinuxSystemCollector>();
#else
    #error "Неподдерживаемая целевая операционная система"
#endif
}

} // namespace pm::core
