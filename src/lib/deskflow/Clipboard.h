#pragma once

#include <string>

namespace deskflow {

class Clipboard {
public:
    Clipboard();
    ~Clipboard();

    void set(const std::string& data);
    std::string get() const;
    void clear();
    bool empty() const;

private:
    std::string m_data;
};

} // namespace deskflow
