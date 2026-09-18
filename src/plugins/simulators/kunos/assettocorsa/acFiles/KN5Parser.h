#pragma once

// KN5Parser.h — Qt-based KN5 parser (full implementation in KN5Types.h + KN5Parser.cpp)
#include "KN5Types.h"

namespace KN5Parser {

// Convenience functions
inline KN5File parseKN5(const QString& path, QString* err = nullptr) {
    return KN5ParserImpl::parse(path, err);
}
inline bool writeKN5(const QString& path, const KN5File& kn5) {
    return KN5ParserImpl::write(path, kn5);
}
inline bool isValidKN5(const QString& path) {
    return KN5ParserImpl::isValid(path);
}
inline QString lastErrorKN5() {
    return KN5ParserImpl::lastError();
}

} // namespace KN5Parser