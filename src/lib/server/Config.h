/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/String.h"
#include "base/XBase.h"
#include "common/stdmap.h"
#include "common/stdset.h"
#include "deskflow/IPlatformScreen.h"
#include "deskflow/option_types.h"
#include "deskflow/protocol_types.h"
#include "net/NetworkAddress.h"
#include "server/InputFilter.h"

#include <iosfwd>

namespace deskflow::server {
class Config;
class ConfigReadContext;
} // namespace deskflow::server

class IEventQueue;

namespace std {
template <> struct iterator_traits<deskflow::server::Config>
{
  using value_type = std::string;
  using difference_type = ptrdiff_t;
  using iterator_category = bidirectional_iterator_tag;
  using pointer = std::string *;
  using reference = std::string &;
};
}; // namespace std

namespace deskflow::server {

//! Server configuration
/*!
This class holds server configuration information.  That includes
the names of screens and their aliases, the links between them,
and network addresses.

Note that case is preserved in screen names but is ignored when
comparing names.  Screen names and their aliases share a
namespace and must be unique.
*/
class Config
{
public:
  using ScreenOptions = std::map<OptionID, OptionValue>;
  using Interval = std::pair<float, float>;

  class CellEdge
  {
  public:
    CellEdge(EDirection side, float position);
    CellEdge(EDirection side, const Interval &);
    CellEdge(const std::string &name, EDirection side, const Interval &);
    ~CellEdge();

    Interval getInterval() const;
    void setName(const std::string &newName);
    std::string getName() const;
    EDirection getSide() const;
    bool overlaps(const CellEdge &) const;
    bool isInside(float x) const;

    // transform position to [0,1]
    float transform(float x) const;

    // transform [0,1] to position
    float inverseTransform(float x) const;

    // compares side and start of interval
    bool operator<(const CellEdge &) const;

    // compares side and interval
    bool operator==(const CellEdge &) const;
    bool operator!=(const CellEdge &) const;

  private:
    void init(const std::string &name, EDirection side, const Interval &);

  private:
    std::string m_name;
    EDirection m_side;
    Interval m_interval;
  };

private:
  class Name
  {
  public:
    Name(Config *, const std::string &name);

    bool operator==(const std::string &name) const;

  private:
    Config *m_config;
    std::string m_name;
  };

  class Cell
  {
  private:
    using EdgeLinks = std::map<CellEdge, CellEdge>;

  public:
    using const_iterator = EdgeLinks::const_iterator;

    bool add(const CellEdge &src, const CellEdge &dst);
    void remove(EDirection side);
    void remove(EDirection side, float position);
    void remove(const Name &destinationName);
    void rename(const Name &oldName, const std::string &newName);

    bool hasEdge(const CellEdge &) const;
    bool overlaps(const CellEdge &) const;

    bool getLink(EDirection side, float position, const CellEdge *&src, const CellEdge *&dst) const;

    bool operator==(const Cell &) const;
    bool operator!=(const Cell &) const;

    const_iterator begin() const;
    const_iterator end() const;

  private:
    EdgeLinks m_neighbors;

  public:
    ScreenOptions m_options;
  };
  using CellMap = std::map<std::string, Cell, deskflow::string::CaselessCmp>;
  using NameMap = std::map<std::string, std::string, deskflow::string::CaselessCmp>;

public:
  using link_const_iterator = Cell::const_iterator;
  using internal_const_iterator = CellMap::const_iterator;
  using all_const_iterator = NameMap::const_iterator;
  class const_iterator : std::iterator_traits<Config>
  {
  public:
    explicit const_iterator() : m_i()
    {
    }
    explicit const_iterator(const internal_const_iterator &i) : m_i(i)
    {
    }
    const_iterator(const const_iterator &src) = default;
    ~const_iterator() = default;

    const_iterator &operator=(const const_iterator &i)
    {
      m_i = i.m_i;
      return *this;
    }
    std::string operator*()
    {
      return m_i->first;
    }
    const std::string *operator->()
    {
      return &(m_i->first);
    }
    const_iterator &operator++()
    {
      ++m_i;
      return *this;
    }
    const_iterator operator++(int)
    {
      return const_iterator(m_i++);
    }
    const_iterator &operator--()
    {
      --m_i;
      return *this;
    }
    const_iterator operator--(int)
    {
      return const_iterator(m_i--);
    }
    bool operator==(const const_iterator &i) const
    {
      return (m_i == i.m_i);
    }
    bool operator!=(const const_iterator &i) const
    {
      return (m_i != i.m_i);
    }

  private:
    internal_const_iterator m_i;
  };

  Config(IEventQueue *events);
  virtual ~Config();

#ifdef TEST_ENV
  Config() : m_inputFilter(NULL)
  {
  }
#endif

  //! @name manipulators
  //@{

  //! Add screen
  /*!
  Adds a screen, returning true iff successful.  If a screen or
  alias with the given name exists then it fails.
  */
  bool addScreen(const std::string &name);

  //! Rename screen
  /*!
  Renames a screen.  All references to the name are updated.
  Returns true iff successful.
  */
  bool renameScreen(const std::string &oldName, const std::string &newName);

  //! Remove screen
  /*!
  Removes a screen.  This also removes aliases for the screen and
  disconnects any connections to the screen.  \c name may be an
  alias.
  */
  void removeScreen(const std::string &name);

  //! Remove all screens
  /*!
  Removes all screens, aliases, and connections.
  */
  void removeAllScreens();

  //! Add alias
  /*!
  Adds an alias for a screen name.  An alias can be used
  any place the canonical screen name can (except addScreen()).
  Returns false if the alias name already exists or the canonical
  name is unknown, otherwise returns true.
  */
  bool addAlias(const std::string &canonical, const std::string &alias);

  //! Remove alias
  /*!
  Removes an alias for a screen name.  It returns false if the
  alias is unknown or a canonical name, otherwise returns true.
  */
  bool removeAlias(const std::string &alias);

  //! Remove aliases
  /*!
  Removes all aliases for a canonical screen name.  It returns false
  if the canonical name is unknown, otherwise returns true.
  */
  bool removeAliases(const std::string &canonical);

  //! Remove all aliases
  /*!
  This removes all aliases but not the screens.
  */
  void removeAllAliases();

  //! Connect screens
  /*!
  Establishes a one-way connection between portions of opposite edges
  of two screens.  Each portion is described by an interval defined
  by two numbers, the start and end of the interval half-open on the
  end.  The numbers range from 0 to 1, inclusive, for the left/top
  to the right/bottom.  The user will be able to jump from the
  \c srcStart to \c srcSend interval of \c srcSide of screen
  \c srcName to the opposite side of screen \c dstName in the interval
  \c dstStart and \c dstEnd when both screens are connected to the
  server and the user isn't locked to a screen.  Returns false if
  \c srcName is unknown.  \c srcStart must be less than or equal to
  \c srcEnd and \c dstStart must be less then or equal to \c dstEnd
  and all of \c srcStart, \c srcEnd, \c dstStart, or \c dstEnd must
  be inside the range [0,1].
  */
  bool connect(
      const std::string &srcName, EDirection srcSide, float srcStart, float srcEnd, const std::string &dstName,
      float dstStart, float dstEnd
  );

  //! Disconnect screens
  /*!
  Removes all connections created by connect() on side \c srcSide.
  Returns false if \c srcName is unknown.
  */
  bool disconnect(const std::string &srcName, EDirection srcSide);

  //! Disconnect screens
  /*!
  Removes the connections created by connect() on side \c srcSide
  covering position \c position.  Returns false if \c srcName is
  unknown.
  */
  bool disconnect(const std::string &srcName, EDirection srcSide, float position);

  //! Set server address
  /*!
  Set the deskflow listen addresses.  There is no default address so
  this must be called to run a server using this configuration.
  */
  void setDeskflowAddress(const NetworkAddress &);

  //! Add a screen option
  /*!
  Adds an option and its value to the named screen.  Replaces the
  existing option's value if there is one.  Returns true iff \c name
  is a known screen.
  */
  bool addOption(const std::string &name, OptionID option, OptionValue value);

  //! Remove a screen option
  /*!
  Removes an option and its value from the named screen.  Does
  nothing if the option doesn't exist on the screen.  Returns true
  iff \c name is a known screen.
  */
  bool removeOption(const std::string &name, OptionID option);

  //! Remove a screen options
  /*!
  Removes all options and values from the named screen.  Returns true
  iff \c name is a known screen.
  */
  bool removeOptions(const std::string &name);

  //! Get the hot key input filter
  /*!
  Returns the hot key input filter.  Clients can modify hotkeys using
  that object.
  */
  virtual InputFilter *getInputFilter();

  //@}
  //! @name accessors
  //@{

  //! Test screen name validity
  /*!
  Returns true iff \c name is a valid screen name.
  */
  bool isValidScreenName(const std::string &name) const;

  //! Get beginning (canonical) screen name iterator
  const_iterator begin() const;
  //! Get ending (canonical) screen name iterator
  const_iterator end() const;

  //! Get beginning screen name iterator
  all_const_iterator beginAll() const;
  //! Get ending screen name iterator
  all_const_iterator endAll() const;

  //! Test for screen name
  /*!
  Returns true iff \c name names a screen.
  */
  virtual bool isScreen(const std::string &name) const;

  //! Test for canonical screen name
  /*!
  Returns true iff \c name is the canonical name of a screen.
  */
  bool isCanonicalName(const std::string &name) const;

  //! Get canonical name
  /*!
  Returns the canonical name of a screen or the empty string if
  the name is unknown.  Returns the canonical name if one is given.
  */
  std::string getCanonicalName(const std::string &name) const;

  //! Get neighbor
  /*!
  Returns the canonical screen name of the neighbor in the given
  direction (set through connect()) at position \c position.  Returns
  the empty string if there is no neighbor in that direction, otherwise
  saves the position on the neighbor in \c positionOut if it's not
  \c NULL.
  */
  std::string getNeighbor(const std::string &, EDirection, float position, float *positionOut) const;

  //! Check for neighbor
  /*!
  Returns \c true if the screen has a neighbor anywhere along the edge
  given by the direction.
  */
  bool hasNeighbor(const std::string &, EDirection) const;

  //! Check for neighbor
  /*!
  Returns \c true if the screen has a neighbor in the given range along
  the edge given by the direction.
  */
  bool hasNeighbor(const std::string &, EDirection, float start, float end) const;

  //! Get beginning neighbor iterator
  link_const_iterator beginNeighbor(const std::string &) const;
  //! Get ending neighbor iterator
  link_const_iterator endNeighbor(const std::string &) const;

  //! Get the server address
  const NetworkAddress &getDeskflowAddress() const;

  //! Get the screen options
  /*!
  Returns all the added options for the named screen.  Returns NULL
  if the screen is unknown and an empty collection if there are no
  options.
  */
  const ScreenOptions *getOptions(const std::string &name) const;

  //! Check for lock to screen action
  /*!
  Returns \c true if this configuration has a lock to screen action.
  This is for backwards compatible support of ScrollLock locking.
  */
  bool hasLockToScreenAction() const;

  //! Compare configurations
  bool operator==(const Config &) const;
  //! Compare configurations
  bool operator!=(const Config &) const;

  //! Read configuration
  /*!
  Reads a configuration from a context.  Throws XConfigRead on error
  and context is unchanged.
  */
  void read(ConfigReadContext &context);

  //! Read configuration
  /*!
  Reads a configuration from a stream.  Throws XConfigRead on error.
  */
  friend std::istream &operator>>(std::istream &, Config &);

  //! Write configuration
  /*!
  Writes a configuration to a stream.
  */
  friend std::ostream &operator<<(std::ostream &, const Config &);

  //! Get direction name
  /*!
  Returns the name of a direction (for debugging).
  */
  static const char *dirName(EDirection);

  //! Get interval as string
  /*!
  Returns an interval as a parseable string.
  */
  static std::string formatInterval(const Interval &);

  //! Get client address as a string
  /*!
   * Return client address a string.
   */
  std::string getClientAddress() const;

  //! Return true if server started in client mode
  /*!
   * In client mode the server initiates connection to client
   */
  bool isClientMode() const;

  //@}

private:
  void readSection(ConfigReadContext &);
  void readSectionOptions(ConfigReadContext &);
  void readSectionScreens(ConfigReadContext &);
  void readSectionLinks(ConfigReadContext &);
  void readSectionAliases(ConfigReadContext &);

  InputFilter::Condition *
  parseCondition(ConfigReadContext &, const std::string &condition, const std::vector<std::string> &args);
  void parseAction(
      ConfigReadContext &, const std::string &action, const std::vector<std::string> &args, InputFilter::Rule &,
      bool activate
  );

  void parseScreens(ConfigReadContext &, const std::string &, std::set<std::string> &screens) const;
  static const char *getOptionName(OptionID);
  static std::string getOptionValue(OptionID, OptionValue);

private:
  CellMap m_map;
  NameMap m_nameToCanonicalName;
  NetworkAddress m_deskflowAddress;
  ScreenOptions m_globalOptions;
  InputFilter m_inputFilter;
  bool m_hasLockToScreenAction;
  IEventQueue *m_events;
  std::string m_ClientAddress;
};

//! Configuration read context
/*!
Maintains a context when reading a configuration from a stream.
*/
class ConfigReadContext
{
public:
  using ArgList = std::vector<std::string>;

  ConfigReadContext(std::istream &, int32_t firstLine = 1);
  ~ConfigReadContext();

  bool readLine(std::string &);
  uint32_t getLineNumber() const;

  bool operator!() const;

  OptionValue parseBoolean(const std::string &) const;
  OptionValue parseInt(const std::string &) const;
  OptionValue parseModifierKey(const std::string &) const;
  OptionValue parseCorner(const std::string &) const;
  OptionValue parseCorners(const std::string &) const;
  OptionValue parseProtocol(const std::string &) const;
  Config::Interval parseInterval(const ArgList &args) const;
  void parseNameWithArgs(
      const std::string &type, const std::string &line, const std::string &delim, std::string::size_type &index,
      std::string &name, ArgList &args
  ) const;
  IPlatformScreen::KeyInfo *parseKeystroke(const std::string &keystroke) const;
  IPlatformScreen::KeyInfo *parseKeystroke(const std::string &keystroke, const std::set<std::string> &screens) const;
  IPlatformScreen::ButtonInfo *parseMouse(const std::string &mouse) const;
  KeyModifierMask parseModifier(const std::string &modifiers) const;
  std::istream &getStream() const
  {
    return m_stream;
  };

private:
  // not implemented
  ConfigReadContext &operator=(const ConfigReadContext &);

  static std::string concatArgs(const ArgList &args);

private:
  std::istream &m_stream;
  int32_t m_line;
};

//! Configuration stream read exception
/*!
Thrown when a configuration stream cannot be parsed.
*/
class XConfigRead : public XBase
{
public:
  XConfigRead(const ConfigReadContext &context, const std::string &);
  XConfigRead(const ConfigReadContext &context, const char *errorFmt, const std::string &arg);
  virtual ~XConfigRead() _NOEXCEPT;

protected:
  // XBase overrides
  virtual std::string getWhat() const throw();

private:
  std::string m_error;
};

} // namespace deskflow::server
