//
// Created by jamie on 21/04/2020.
//

#include "ConfigWriter.h"

ConfigWriter* ConfigWriter::s_pConfiguration = nullptr;


ConfigWriter *ConfigWriter::make() {
    // Only one ConfigWriter can exist at any one time (Singolton)
    if (!s_pConfiguration) {
        s_pConfiguration = new ConfigWriter();
    }
    return s_pConfiguration;
}

ConfigWriter::ConfigWriter() {

}

void ConfigWriter::destroy() {
    destroy(s_pConfiguration);
}

ConfigWriter::~ConfigWriter() {
    destroy(m_pSettingsCurrent);
    destroy(m_pSettingsSystem);
    destroy(m_pSettingsUser);
}

template<typename T>
void ConfigWriter::setSetting(const char *name, T value) {

}

QVariant ConfigWriter::loadSetting(const char *name, const QVariant &defaultValue) {
    return QVariant();
}


void ConfigWriter::setScope(ConfigWriter::Scope scope) {
    if (m_CurrentScope != scope)
    {
        m_CurrentScope = scope;
        switch (scope)
        {
            case User:
                m_pSettingsCurrent = m_pSettingsUser;
                break;
            case System:
                m_pSettingsCurrent = m_pSettingsSystem;
                break;
        }
    }
}

ConfigWriter::Scope ConfigWriter::getScope() const {
    return m_CurrentScope;
}

void ConfigWriter::gloablLoad() {
    for(auto & i : m_pCallerList) {
        i->loadSettings();
    }
}

void ConfigWriter::globalSave() {
    for(auto & i : m_pCallerList) {
        i->saveSettings();
    }
}

