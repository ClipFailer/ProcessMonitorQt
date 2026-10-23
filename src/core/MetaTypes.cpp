#include "MetaTypes.hpp"
#include "Types.hpp"

namespace pm::core {

void registerMetaTypes() {
    qRegisterMetaType<pm::core::ProcessSnapshot>("pm::core::ProcessSnapshot");
    qRegisterMetaType<std::vector<pm::core::ProcessSnapshot>>("std::vector<pm::core::ProcessSnapshot>");
    qRegisterMetaType<pm::core::SystemSnapshot>("pm::core::SystemSnapshot");
    qRegisterMetaType<pm::core::ThreadSnapshot>("pm::core::ThreadSnapshot");
    qRegisterMetaType<std::vector<pm::core::ThreadSnapshot>>("std::vector<pm::core::ThreadSnapshot>");
    qRegisterMetaType<pm::core::ModuleSnapshot>("pm::core::ModuleSnapshot");
    qRegisterMetaType<std::vector<pm::core::ModuleSnapshot>>("std::vector<pm::core::ModuleSnapshot>");
    qRegisterMetaType<pm::core::HandleSnapshot>("pm::core::HandleSnapshot");
    qRegisterMetaType<std::vector<pm::core::HandleSnapshot>>("std::vector<pm::core::HandleSnapshot>");
}

} // namespace pm::core
