/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace deskflow {

//! Format preference and quality information
struct FormatPreference
{
  //! Deskflow clipboard format
  IClipboard::EFormat format;

  //! MIME type
  std::string mimeType;

  //! Quality score (0.0 to 1.0, higher is better)
  double quality;

  //! Size efficiency (bytes per unit of information)
  double efficiency;

  //! Whether this format preserves all information
  bool lossless;

  //! Platform compatibility score
  double compatibility;

  FormatPreference(
      IClipboard::EFormat fmt, const std::string &mime, double qual = 1.0, double eff = 1.0, bool loss = true,
      double compat = 1.0
  )
      : format(fmt),
        mimeType(mime),
        quality(qual),
        efficiency(eff),
        lossless(loss),
        compatibility(compat)
  {
  }
};

//! Clipboard format negotiation and selection
/*!
This class handles intelligent format selection and negotiation between
different clipboard formats. It considers quality, efficiency, compatibility,
and user preferences to select the best format for clipboard operations.
*/
class EiClipboardNegotiator
{
public:
  //! Negotiation strategy
  enum class Strategy
  {
    Quality,       // Prioritize highest quality
    Efficiency,    // Prioritize smallest size
    Compatibility, // Prioritize best compatibility
    Balanced,      // Balance all factors
    Custom         // Use custom scoring function
  };

  //! Custom scoring function type
  using ScoringFunction = std::function<double(const FormatPreference &)>;

  EiClipboardNegotiator();
  ~EiClipboardNegotiator();

  //! Set negotiation strategy
  void setStrategy(Strategy strategy);

  //! Set custom scoring function
  void setCustomScoring(ScoringFunction scoringFunc);

  //! Get current strategy
  Strategy getStrategy() const;

  //! Add format preference
  void addFormatPreference(const FormatPreference &preference);

  //! Remove format preference
  void removeFormatPreference(IClipboard::EFormat format, const std::string &mimeType);

  //! Clear all format preferences
  void clearFormatPreferences();

  //! Get all format preferences for a format
  std::vector<FormatPreference> getFormatPreferences(IClipboard::EFormat format) const;

  //! Negotiate best format from available options
  FormatPreference
  negotiateBestFormat(IClipboard::EFormat desiredFormat, const std::vector<std::string> &availableMimeTypes) const;

  //! Negotiate best format from available formats
  FormatPreference negotiateBestFormat(const std::vector<IClipboard::EFormat> &availableFormats) const;

  //! Select best MIME type for a format
  std::string selectBestMimeType(IClipboard::EFormat format, const std::vector<std::string> &availableMimeTypes) const;

  //! Calculate format score based on current strategy
  double calculateFormatScore(const FormatPreference &preference) const;

  //! Check if format conversion is needed
  bool needsConversion(
      IClipboard::EFormat sourceFormat, const std::string &sourceMimeType, IClipboard::EFormat targetFormat,
      const std::string &targetMimeType
  ) const;

  //! Get format conversion priority
  int getConversionPriority(IClipboard::EFormat sourceFormat, IClipboard::EFormat targetFormat) const;

  //! Set format weights for balanced strategy
  struct FormatWeights
  {
    double quality = 0.4;
    double efficiency = 0.2;
    double compatibility = 0.3;
    double lossless = 0.1;
  };

  void setFormatWeights(const FormatWeights &weights);
  const FormatWeights &getFormatWeights() const;

  //! Get default format preferences
  static std::vector<FormatPreference> getDefaultPreferences();

  //! Load format preferences from configuration
  void loadPreferences(const std::map<std::string, std::string> &config);

  //! Save format preferences to configuration
  std::map<std::string, std::string> savePreferences() const;

private:
  //! Initialize default format preferences
  void initializeDefaults();

  //! Calculate quality score
  double calculateQualityScore(const FormatPreference &preference) const;

  //! Calculate efficiency score
  double calculateEfficiencyScore(const FormatPreference &preference) const;

  //! Calculate compatibility score
  double calculateCompatibilityScore(const FormatPreference &preference) const;

  //! Calculate balanced score
  double calculateBalancedScore(const FormatPreference &preference) const;

  Strategy m_strategy;
  ScoringFunction m_customScoring;
  FormatWeights m_weights;

  // Format preferences organized by format
  std::map<IClipboard::EFormat, std::vector<FormatPreference>> m_preferences;

  // MIME type to format mapping
  std::map<std::string, IClipboard::EFormat> m_mimeToFormat;

  // Format conversion matrix (source -> target priority)
  std::map<std::pair<IClipboard::EFormat, IClipboard::EFormat>, int> m_conversionPriority;
};

} // namespace deskflow
