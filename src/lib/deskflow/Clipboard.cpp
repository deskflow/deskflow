#include "Clipboard.h"

namespace deskflow {

Clipboard::Clipboard() = default;
Clipboard::~Clipboard() = default;

void Clipboard::set(const std::string& data) {
    m_data = data;
}

std::string Clipboard::get() const {
    return m_data;
}

void Clipboard::clear() {
    m_data.clear();
}

bool Clipboard::empty() const {
    return m_data.empty();
}

} // namespace deskflow
