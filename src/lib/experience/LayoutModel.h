// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <string>
#include <vector>

namespace deskflow::experience {
// Whole-computer logical desktop rectangles. This is NOT per-monitor input topology.
struct Desktop
{
  std::string name;
  double x, y, width, height;
  std::string displayName = {}; // Local GUI metadata; never exported as a protocol identifier.
  bool operator==(const Desktop &) const = default;
};
enum class Edge
{
  Left,
  Right,
  Up,
  Down
};
struct EdgeLink
{
  std::string from, to;
  Edge edge;
  double sourceStart, sourceEnd, targetStart, targetEnd; // percentages, 0..100
};
struct LayoutValidation
{
  std::vector<std::string> errors;
  std::vector<EdgeLink> links;
  bool valid() const
  {
    return errors.empty();
  }
};
LayoutValidation validateLayout(const std::vector<Desktop> &desktops);
std::string exportLinks(const std::vector<Desktop> &desktops);

class LayoutModel
{
public:
  explicit LayoutModel(std::vector<Desktop> desktops, bool allowInvalidDraft = false);
  void move(const std::string &name, double x, double y, bool snap = true, double threshold = 16);
  void resize(const std::string &name, double width, double height);
  void setDisplayName(const std::string &name, const std::string &displayName);
  bool undo();
  bool canUndo() const
  {
    return !m_history.empty();
  }
  void cancel();
  bool apply(); // in-memory only; exporter is explicit and has no filesystem access
  const std::vector<Desktop> &desktops() const
  {
    return m_working;
  }
  LayoutValidation validate() const
  {
    return validateLayout(m_working);
  }

private:
  std::vector<Desktop> m_working, m_saved;
  std::vector<std::vector<Desktop>> m_history;
};
} // namespace deskflow::experience
