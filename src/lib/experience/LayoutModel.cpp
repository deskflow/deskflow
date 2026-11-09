// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "LayoutModel.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>

namespace deskflow::experience {
namespace {
constexpr double epsilon = 0.000001;
bool near(double a, double b)
{
  return std::abs(a - b) <= epsilon;
}
bool validName(const std::string &name)
{
  return !name.empty() && name.size() <= 128 && std::all_of(name.begin(), name.end(), [](unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
           c == '.';
  });
}
bool coordinate(double x)
{
  return std::isfinite(x) && std::abs(x) <= 10000000;
}
bool dimensions(const Desktop &d)
{
  return coordinate(d.x) && coordinate(d.y) && std::isfinite(d.width) && std::isfinite(d.height) && d.width >= 1 &&
         d.height >= 1 && d.width <= 1000000 && d.height <= 1000000;
}
double overlap(double a, double lengthA, double b, double lengthB)
{
  return std::min(a + lengthA, b + lengthB) - std::max(a, b);
}
const char *edgeName(Edge edge)
{
  switch (edge) {
  case Edge::Left:
    return "left";
  case Edge::Right:
    return "right";
  case Edge::Up:
    return "up";
  case Edge::Down:
    return "down";
  }
  throw std::invalid_argument("invalid edge");
}
void addLinks(std::vector<EdgeLink> &links, const Desktop &a, const Desktop &b, Edge edge, Edge reverse, bool vertical)
{
  const double aStart = vertical ? a.y : a.x, bStart = vertical ? b.y : b.x;
  const double aSize = vertical ? a.height : a.width, bSize = vertical ? b.height : b.width;
  const double low = std::max(aStart, bStart), high = std::min(aStart + aSize, bStart + bSize);
  if (high - low <= epsilon)
    return; // a corner does not provide a usable crossing interval
  const auto percent = [](double x, double start, double size) {
    return std::clamp((x - start) / size * 100, 0.0, 100.0);
  };
  EdgeLink l{
      a.name,
      b.name,
      edge,
      percent(low, aStart, aSize),
      percent(high, aStart, aSize),
      percent(low, bStart, bSize),
      percent(high, bStart, bSize)
  };
  links.push_back(l);
  links.push_back({b.name, a.name, reverse, l.targetStart, l.targetEnd, l.sourceStart, l.sourceEnd});
}
} // namespace

LayoutValidation validateLayout(const std::vector<Desktop> &desktops)
{
  LayoutValidation result;
  if (desktops.empty() || desktops.size() > 16) {
    result.errors.push_back("layout requires 1..16 computers");
    return result;
  }
  std::set<std::string> names;
  for (const auto &d : desktops) {
    auto folded = d.name;
    std::transform(folded.begin(), folded.end(), folded.begin(), [](unsigned char c) {
      return static_cast<char>(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
    });
    if (!validName(d.name) || !dimensions(d) || !names.insert(folded).second)
      result.errors.push_back("invalid geometry or duplicate/unsupported computer name");
  }
  if (!result.valid())
    return result;
  for (std::size_t i = 0; i < desktops.size(); ++i) {
    for (std::size_t j = i + 1; j < desktops.size(); ++j) {
      const auto &a = desktops[i];
      const auto &b = desktops[j];
      if (overlap(a.x, a.width, b.x, b.width) > epsilon && overlap(a.y, a.height, b.y, b.height) > epsilon) {
        result.errors.push_back("desktops overlap: " + a.name + ", " + b.name);
        continue;
      }
      if (near(a.x + a.width, b.x))
        addLinks(result.links, a, b, Edge::Right, Edge::Left, true);
      if (near(a.x, b.x + b.width))
        addLinks(result.links, a, b, Edge::Left, Edge::Right, true);
      if (near(a.y + a.height, b.y))
        addLinks(result.links, a, b, Edge::Down, Edge::Up, false);
      if (near(a.y, b.y + b.height))
        addLinks(result.links, a, b, Edge::Up, Edge::Down, false);
    }
  }
  // The existing core stores normalized edge intervals as float. Reject links
  // that collapse there, even when their double-precision geometry is nonzero.
  for (const auto &link : result.links) {
    if (static_cast<float>(link.sourceStart / 100) >= static_cast<float>(link.sourceEnd / 100) ||
        static_cast<float>(link.targetStart / 100) >= static_cast<float>(link.targetEnd / 100))
      result.errors.push_back("shared edge is too small for the core to represent");
  }
  std::set<std::string> reachable{desktops.front().name};
  for (std::size_t i = 0; i < desktops.size(); ++i)
    for (const auto &link : result.links)
      if (reachable.contains(link.from))
        reachable.insert(link.to);
  if (reachable.size() != desktops.size())
    result.errors.push_back("every computer must share a usable edge with the connected layout");
  return result;
}

std::string exportLinks(const std::vector<Desktop> &desktops)
{
  const auto result = validateLayout(desktops);
  if (!result.valid())
    throw std::invalid_argument("cannot export an invalid layout");
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(17) << "# Whole-computer logical desktop layout.\nsection: links\n";
  for (const auto &d : desktops) {
    out << "  " << d.name << ":\n";
    for (const auto &l : result.links)
      if (l.from == d.name)
        out << "    " << edgeName(l.edge) << '(' << l.sourceStart << ',' << l.sourceEnd << ") = " << l.to << '('
            << l.targetStart << ',' << l.targetEnd << ")\n";
  }
  out << "end\n";
  return out.str();
}

LayoutModel::LayoutModel(std::vector<Desktop> desktops, bool allowInvalidDraft)
    : m_working(std::move(desktops)),
      m_saved(m_working)
{
  if (!allowInvalidDraft && !validate().valid())
    throw std::invalid_argument("initial layout must be valid");
}
void LayoutModel::setDisplayName(const std::string &name, const std::string &displayName)
{
  auto it = std::find_if(m_working.begin(), m_working.end(), [&](const Desktop &d) { return d.name == name; });
  if (it == m_working.end())
    throw std::invalid_argument("unknown computer");
  if (it->displayName == displayName)
    return;
  m_history.push_back(m_working);
  it->displayName = displayName;
}
void LayoutModel::resize(const std::string &name, double width, double height)
{
  auto it = std::find_if(m_working.begin(), m_working.end(), [&name](const Desktop &d) { return d.name == name; });
  if (it == m_working.end())
    throw std::invalid_argument("unknown computer");
  Desktop next = *it;
  next.width = width;
  next.height = height;
  if (!dimensions(next))
    throw std::invalid_argument("invalid desktop size");
  if (next == *it)
    return;
  if (m_history.size() == 64)
    m_history.erase(m_history.begin());
  m_history.push_back(m_working);
  *it = next;
}
void LayoutModel::move(const std::string &name, double x, double y, bool snap, double threshold)
{
  if (!coordinate(x) || !coordinate(y) || !std::isfinite(threshold) || threshold < 0 || threshold > 1000)
    throw std::invalid_argument("invalid move");
  auto it = std::find_if(m_working.begin(), m_working.end(), [&name](const Desktop &d) { return d.name == name; });
  if (it == m_working.end())
    throw std::invalid_argument("unknown computer");
  Desktop next = *it;
  next.x = x;
  next.y = y;
  if (snap) {
    double best = threshold + epsilon;
    Desktop snapped = next;
    for (const auto &other : m_working) {
      if (other.name == name)
        continue;
      if (overlap(next.y, next.height, other.y, other.height) > epsilon) {
        for (double target : {other.x - next.width, other.x + other.width})
          if (std::abs(target - x) < best) {
            best = std::abs(target - x);
            snapped = next;
            snapped.x = target;
          }
      }
      if (overlap(next.x, next.width, other.x, other.width) > epsilon) {
        for (double target : {other.y - next.height, other.y + other.height})
          if (std::abs(target - y) < best) {
            best = std::abs(target - y);
            snapped = next;
            snapped.y = target;
          }
      }
    }
    next = snapped;
  }
  if (next == *it)
    return;
  if (m_history.size() == 64)
    m_history.erase(m_history.begin());
  m_history.push_back(m_working);
  *it = next; // Invalid intermediate placement is visible; apply/export remains blocked.
}
bool LayoutModel::undo()
{
  if (m_history.empty())
    return false;
  m_working = std::move(m_history.back());
  m_history.pop_back();
  return true;
}
void LayoutModel::cancel()
{
  m_working = m_saved;
  m_history.clear();
}
bool LayoutModel::apply()
{
  if (!validate().valid())
    return false;
  m_saved = m_working;
  m_history.clear();
  return true;
}
} // namespace deskflow::experience
