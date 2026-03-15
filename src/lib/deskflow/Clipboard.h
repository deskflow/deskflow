#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace deskflow {

class IClipboard {
public:
    virtual ~IClipboard() = default;
    virtual bool empty() = 0;
    virtual void add(const std::string& mimeType, const std::string& data) = 0;
    virtual bool open() const = 0;
    virtual void close() const = 0;
    virtual std::string get(const std::string& mimeType) const = 0;
};

class Clipboard : public IClipboard {
public:
    Clipboard();
    ~Clipboard() override;

    bool empty() override;
    void add(const std::string& mimeType, const std::string& data) override;
    bool open() const override;
    void close() const override;
    std::string get(const std::string& mimeType) const override;

    // XDG Desktop Portal integration
    void setPortalClipboard(bool enabled);
    bool isPortalClipboardEnabled() const;

private:
    class Impl;
    Impl* m_impl;
    bool m_portalEnabled;
};

} // namespace deskflow
