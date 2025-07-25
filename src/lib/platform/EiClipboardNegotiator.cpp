/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboardNegotiator.h"
#include "base/Log.h"

#include <algorithm>
#include <sstream>

namespace deskflow {

EiClipboardNegotiator::EiClipboardNegotiator() : m_strategy(Strategy::Balanced)
{
  initializeDefaults();
}

EiClipboardNegotiator::~EiClipboardNegotiator() = default;

void EiClipboardNegotiator::setStrategy(Strategy strategy)
{
  m_strategy = strategy;
  LOG_DEBUG("clipboard negotiation strategy set to %d", static_cast<int>(strategy));
}

void EiClipboardNegotiator::setCustomScoring(ScoringFunction scoringFunc)
{
  m_customScoring = std::move(scoringFunc);
  m_strategy = Strategy::Custom;
}

EiClipboardNegotiator::Strategy EiClipboardNegotiator::getStrategy() const
{
  return m_strategy;
}

void EiClipboardNegotiator::addFormatPreference(const FormatPreference &preference)
{
  m_preferences[preference.format].push_back(preference);
  m_mimeToFormat[preference.mimeType] = preference.format;

  LOG_DEBUG(
      "added format preference: format=%d, mime=%s, quality=%.2f", preference.format, preference.mimeType.c_str(),
      preference.quality
  );
}

void EiClipboardNegotiator::removeFormatPreference(IClipboard::EFormat format, const std::string &mimeType)
{
  auto it = m_preferences.find(format);
  if (it != m_preferences.end()) {
    auto &prefs = it->second;
    prefs.erase(
        std::remove_if(
            prefs.begin(), prefs.end(), [&mimeType](const FormatPreference &pref) { return pref.mimeType == mimeType; }
        ),
        prefs.end()
    );

    if (prefs.empty()) {
      m_preferences.erase(it);
    }
  }

  m_mimeToFormat.erase(mimeType);
}

void EiClipboardNegotiator::clearFormatPreferences()
{
  m_preferences.clear();
  m_mimeToFormat.clear();
  initializeDefaults();
}

std::vector<FormatPreference> EiClipboardNegotiator::getFormatPreferences(IClipboard::EFormat format) const
{
  auto it = m_preferences.find(format);
  return (it != m_preferences.end()) ? it->second : std::vector<FormatPreference>{};
}

FormatPreference EiClipboardNegotiator::negotiateBestFormat(
    IClipboard::EFormat desiredFormat, const std::vector<std::string> &availableMimeTypes
) const
{
  auto preferences = getFormatPreferences(desiredFormat);
  if (preferences.empty()) {
    // Return default preference if none found
    return FormatPreference(desiredFormat, "application/octet-stream", 0.1);
  }

  FormatPreference bestPreference = preferences[0];
  double bestScore = 0.0;

  for (const auto &preference : preferences) {
    // Check if this MIME type is available
    if (std::find(availableMimeTypes.begin(), availableMimeTypes.end(), preference.mimeType) ==
        availableMimeTypes.end()) {
      continue;
    }

    double score = calculateFormatScore(preference);
    if (score > bestScore) {
      bestScore = score;
      bestPreference = preference;
    }
  }

  LOG_DEBUG("negotiated format: %s (score: %.2f)", bestPreference.mimeType.c_str(), bestScore);
  return bestPreference;
}

FormatPreference
EiClipboardNegotiator::negotiateBestFormat(const std::vector<IClipboard::EFormat> &availableFormats) const
{
  FormatPreference bestPreference(IClipboard::kText, "text/plain", 0.1);
  double bestScore = 0.0;

  for (auto format : availableFormats) {
    auto preferences = getFormatPreferences(format);
    for (const auto &preference : preferences) {
      double score = calculateFormatScore(preference);
      if (score > bestScore) {
        bestScore = score;
        bestPreference = preference;
      }
    }
  }

  return bestPreference;
}

std::string EiClipboardNegotiator::selectBestMimeType(
    IClipboard::EFormat format, const std::vector<std::string> &availableMimeTypes
) const
{
  auto bestFormat = negotiateBestFormat(format, availableMimeTypes);
  return bestFormat.mimeType;
}

double EiClipboardNegotiator::calculateFormatScore(const FormatPreference &preference) const
{
  switch (m_strategy) {
  case Strategy::Quality:
    return calculateQualityScore(preference);
  case Strategy::Efficiency:
    return calculateEfficiencyScore(preference);
  case Strategy::Compatibility:
    return calculateCompatibilityScore(preference);
  case Strategy::Balanced:
    return calculateBalancedScore(preference);
  case Strategy::Custom:
    return m_customScoring ? m_customScoring(preference) : preference.quality;
  default:
    return preference.quality;
  }
}

bool EiClipboardNegotiator::needsConversion(
    IClipboard::EFormat sourceFormat, const std::string &sourceMimeType, IClipboard::EFormat targetFormat,
    const std::string &targetMimeType
) const
{
  return sourceFormat != targetFormat || sourceMimeType != targetMimeType;
}

int EiClipboardNegotiator::getConversionPriority(
    IClipboard::EFormat sourceFormat, IClipboard::EFormat targetFormat
) const
{
  auto key = std::make_pair(sourceFormat, targetFormat);
  auto it = m_conversionPriority.find(key);
  return (it != m_conversionPriority.end()) ? it->second : 0;
}

void EiClipboardNegotiator::setFormatWeights(const FormatWeights &weights)
{
  m_weights = weights;
  LOG_DEBUG(
      "format weights updated: quality=%.2f, efficiency=%.2f, compatibility=%.2f, lossless=%.2f", weights.quality,
      weights.efficiency, weights.compatibility, weights.lossless
  );
}

const EiClipboardNegotiator::FormatWeights &EiClipboardNegotiator::getFormatWeights() const
{
  return m_weights;
}

std::vector<FormatPreference> EiClipboardNegotiator::getDefaultPreferences()
{
  return {
      // Text formats (highest quality first)
      FormatPreference(IClipboard::kText, "text/plain;charset=utf-8", 1.0, 0.9, true, 1.0),
      FormatPreference(IClipboard::kText, "text/plain", 0.9, 0.9, true, 1.0),
      FormatPreference(IClipboard::kText, "UTF8_STRING", 0.8, 0.9, true, 0.8),
      FormatPreference(IClipboard::kText, "STRING", 0.7, 0.9, false, 0.9),

      // HTML formats
      FormatPreference(IClipboard::kHTML, "text/html;charset=utf-8", 1.0, 0.7, true, 0.9),
      FormatPreference(IClipboard::kHTML, "text/html", 0.9, 0.7, true, 0.9),
      FormatPreference(IClipboard::kHTML, "application/xhtml+xml", 0.8, 0.6, true, 0.7),

      // Image formats (lossless first)
      FormatPreference(IClipboard::kBitmap, "image/png", 1.0, 0.5, true, 0.9),
      FormatPreference(IClipboard::kBitmap, "image/bmp", 0.9, 0.3, true, 0.8),
      FormatPreference(IClipboard::kBitmap, "image/tiff", 0.8, 0.4, true, 0.7),
      FormatPreference(IClipboard::kBitmap, "image/jpeg", 0.7, 0.8, false, 0.9),
      FormatPreference(IClipboard::kBitmap, "image/gif", 0.6, 0.7, false, 0.8),
  };
}

void EiClipboardNegotiator::loadPreferences(const std::map<std::string, std::string> &config)
{
  // TODO: Implement configuration loading
  LOG_DEBUG("loading clipboard format preferences from configuration");
}

std::map<std::string, std::string> EiClipboardNegotiator::savePreferences() const
{
  // TODO: Implement configuration saving
  LOG_DEBUG("saving clipboard format preferences to configuration");
  return {};
}

void EiClipboardNegotiator::initializeDefaults()
{
  clearFormatPreferences();

  auto defaults = getDefaultPreferences();
  for (const auto &pref : defaults) {
    addFormatPreference(pref);
  }

  // Initialize conversion priorities
  // Higher numbers = higher priority
  m_conversionPriority[{IClipboard::kText, IClipboard::kHTML}] = 3;
  m_conversionPriority[{IClipboard::kHTML, IClipboard::kText}] = 2;
  m_conversionPriority[{IClipboard::kText, IClipboard::kBitmap}] = 1;
  m_conversionPriority[{IClipboard::kBitmap, IClipboard::kText}] = 1;
  m_conversionPriority[{IClipboard::kHTML, IClipboard::kBitmap}] = 1;
  m_conversionPriority[{IClipboard::kBitmap, IClipboard::kHTML}] = 1;
}

double EiClipboardNegotiator::calculateQualityScore(const FormatPreference &preference) const
{
  return preference.quality;
}

double EiClipboardNegotiator::calculateEfficiencyScore(const FormatPreference &preference) const
{
  return preference.efficiency;
}

double EiClipboardNegotiator::calculateCompatibilityScore(const FormatPreference &preference) const
{
  return preference.compatibility;
}

double EiClipboardNegotiator::calculateBalancedScore(const FormatPreference &preference) const
{
  double score = 0.0;
  score += preference.quality * m_weights.quality;
  score += preference.efficiency * m_weights.efficiency;
  score += preference.compatibility * m_weights.compatibility;
  score += (preference.lossless ? 1.0 : 0.0) * m_weights.lossless;
  return score;
}

} // namespace deskflow
