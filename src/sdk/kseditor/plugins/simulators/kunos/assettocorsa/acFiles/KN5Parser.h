#pragma once

// KN5Parser.h — Qt-based KN5 parser (full implementation in KN5Types.h + KN5Parser.cpp)
#include "KN5Types.h"

// Convenience functions in KN5Parser namespace (matching KN5Types.h)
inline KN5Parser::KN5File parseKN5(const QString& path, QString* err = nullptr) {
    return KN5Parser::KN5ParserImpl::parse(path, err);
}
inline bool writeKN5(const QString& path, const KN5Parser::KN5File& kn5) {
    return KN5Parser::KN5ParserImpl::write(path, kn5);
}
inline bool isValidKN5(const QString& path) {
    return KN5Parser::KN5ParserImpl::isValid(path);
}
inline QString lastErrorKN5() {
    return KN5Parser::KN5ParserImpl::lastError();
}