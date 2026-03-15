#pragma once

#include <string>
#include <unordered_map>

namespace deskflow {

class IClipboard
{
public:
    virtual ~IClipboard() = default;
    virtual void clear() = 0;
    virtual bool hasData() const = 0;
    virtual std::string getData(const std::string& format) const = 0;
    virtual void setData(const std::string& format, const std::string& data) = 0;
};

class Clipboard : public IClipboard
{
public:
    void clear() override { m_data.clear(); }

    bool hasData() const override { return !m_data.empty(); }

    std::string getData(const std::string& format) const override
    {
        auto it = m_data.find(format);
        return (it != m_data.end()) ? it->second : std::string();
    }

    void setData(const std::string& format, const std::string& data) override
    {
        m_data[format] = data;
    }

private:
    std::unordered_map<std::string, std::string> m_data;
};

} // namespace deskflow
