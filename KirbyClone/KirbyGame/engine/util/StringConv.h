#pragma once
//
// Responsibility: UTF-8 <-> UTF-16 conversion helpers.
// Non-Goals:      Locale-specific encodings, error reporting.
// Call-Context:   Any thread; small/throw-free helpers.
//

#include <string>
#include <string_view>

namespace engine {

	std::wstring ToWide ( std::string_view s );
	std::string  ToUtf8 ( std::wstring_view ws );

} // namespace engine
