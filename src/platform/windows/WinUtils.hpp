#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winternl.h>
#include <string>
#include <cstdint>

namespace pm::platform::win {

/**
 * @brief Преобразование структуры FILETIME в 64-битное целое число (100-нс интервалы).
 * @param[in] ft Структура FILETIME.
 * @return std::uint64_t Количество 100-наносекундных интервалов с 1 января 1601 года.
 */
[[nodiscard]] inline std::uint64_t fileTimeToUint64(const FILETIME& ft) noexcept {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

/**
 * @brief Конвертация UTF-16 строки (std::wstring) в UTF-8 (std::string).
 * @param[in] wstr Входная строка в кодировке UTF-16.
 * @return std::string Результирующая строка UTF-8.
 */
inline std::string wideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) {
        return {};
    }
    const int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
        nullptr, 0, nullptr, nullptr
    );
    if (sizeNeeded <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(sizeNeeded), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
        result.data(), sizeNeeded, nullptr, nullptr
    );
    return result;
}

/**
 * @brief Включение привилегии SeDebugPrivilege в текущем маркере безопасности процесса.
 * @details Позволяет открывать системные процессы и службы с правами PROCESS_QUERY_LIMITED_INFORMATION
 * и PROCESS_VM_READ для получения полной информации о памяти, дескрипторах и командной строке.
 * @return true если привилегия успешно активирована, иначе false.
 */
inline bool enableDebugPrivilege() noexcept {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    const BOOL status = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    const DWORD error = GetLastError();
    CloseHandle(hToken);

    return (status != FALSE && error != ERROR_NOT_ALL_ASSIGNED);
}

/**
 * @brief Проверка запуска процесса от имени учетной записи с правами Администратора.
 * @return true если процесс имеет токен локального администратора, иначе false.
 */
inline bool isUserAdmin() noexcept {
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;
    if (!AllocateAndInitializeSid(
            &ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        return false;
    }

    BOOL isAdmin = FALSE;
    CheckTokenMembership(nullptr, adminGroup, &isAdmin);
    FreeSid(adminGroup);
    return isAdmin != FALSE;
}

} // namespace pm::platform::win

#endif // _WIN32
