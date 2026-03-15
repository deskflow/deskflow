#include "Clipboard.h"
#include <algorithm>
#include <map>

namespace deskflow {

class Clipboard::Impl {
public:
    std::map<std::string, std::string> m_data;
    bool m_opened = false;
};

Clipboard::Clipboard() : m_impl(new Impl), m_portalEnabled(false) {
}

Clipboard::~Clipboard() {
    delete m_impl;
}

bool Clipboard::empty() {
    m_impl->m_data.clear();
    return true;
}

void Clipboard::add(const std::string& mimeType, const std::string& data) {
    m_impl->m_data[mimeType] = data;
}

bool Clipboard::open() const {
    m_impl->m_opened = true;
    return true;
}

void Clipboard::close() const {
    m_impl->m_opened = false;
}

std::string Clipboard::get(const std::string& mimeType) const {
    auto it = m_impl->m_data.find(mimeType);
    if (it != m_impl->m_data.end()) {
        return it->second;
    }
    return "";
}

void Clipboard::setPortalClipboard(bool enabled) {
    m_portalEnabled = enabled;
}

bool Clipboard::isPortalClipboardEnabled() const {
    return m_portalEnabled;
}

} // namespace deskflow
