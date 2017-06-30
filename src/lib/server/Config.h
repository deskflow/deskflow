/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "server/InputFilter.h"
#include "synergy/option_types.h"
#include "synergy/protocol_types.h"
#include "synergy/IPlatformScreen.h"
#include "net/NetworkAddress.h"
#include "base/String.h"
#include "base/XBase.h"
#include "common/stdmap.h"
#include "common/stdset.h"

#include <iosfwd>

class Config;
class ConfigReadContext;
class IEventQueue;

namespace std {
template <>
struct iterator_traits<Config> {
    typedef String value_type;
    typedef ptrdiff_t difference_type;
    typedef bidirectional_iterator_tag iterator_category;
    typedef String* pointer;
    typedef String& reference;
};
};

//! Server configuration
/*!
This class holds server configuration information.  That includes
the names of screens and their aliases, the links between them,
and network addresses.

Note that case is preserved in screen names but is ignored when
comparing names.  Screen names and their aliases share a
namespace and must be unique.
*/
class Config {
public:
    typedef std::map<OptionID, OptionValue> ScreenOptions;
    typedef std::pair<float, float> Interval;

    class CellEdge {
    public:
        CellEdge (EDirection side, float position);
        CellEdge (EDirection side, const Interval&);
        CellEdge (const String& name, EDirection side, const Interval&);
        ~CellEdge ();

        Interval getInterval () const;
        void setName (const String& newName);
        String getName () const;
        EDirection getSide () const;
        bool overlaps (const CellEdge&) const;
        bool isInside (float x) const;

        // transform position to [0,1]
        float transform (float x) const;

        // transform [0,1] to position
        float inverseTransform (float x) const;

        // compares side and start of interval
        bool operator< (const CellEdge&) const;

        // compares side and interval
        bool operator== (const CellEdge&) const;
        bool operator!= (const CellEdge&) const;

    private:
        void init (const String& name, EDirection side, const Interval&);

    private:
        String m_name;
        EDirection m_side;
        Interval m_interval;
    };

private:
    class Name {
    public:
        Name (Config*, const String& name);

        bool operator== (const String& name) const;

    private:
        Config* m_config;
        String m_name;
    };

    class Cell {
    private:
        typedef std::map<CellEdge, CellEdge> EdgeLinks;

    public:
        typedef EdgeLinks::const_iterator const_iterator;

        bool add (const CellEdge& src, const CellEdge& dst);
        void remove (EDirection side);
        void remove (EDirection side, float position);
        void remove (const Name& destinationName);
        void rename (const Name& oldName, const String& newName);

        bool hasEdge (const CellEdge&) const;
        bool overlaps (const CellEdge&) const;

        bool getLink (EDirection side, float position, const CellEdge*& src,
                      const CellEdge*& dst) const;

        bool operator== (const Cell&) const;
        bool operator!= (const Cell&) const;

        const_iterator begin () const;
        const_iterator end () const;

    private:
        EdgeLinks m_neighbors;

    public:
        ScreenOptions m_options;
    };
    typedef std::map<String, Cell, synergy::string::CaselessCmp> CellMap;
    typedef std::map<String, String, synergy::string::CaselessCmp> NameMap;

public:
    typedef Cell::const_iterator link_const_iterator;
    typedef CellMap::const_iterator internal_const_iterator;
    typedef NameMap::const_iterator all_const_iterator;
    class const_iterator : std::iterator_traits<Config> {
    public:
        explicit const_iterator () : m_i () {
        }
        explicit const_iterator (const internal_const_iterator& i) : m_i (i) {
        }

        const_iterator&
        operator= (const const_iterator& i) {
            m_i = i.m_i;
            return *this;
        }
        String operator* () {
            return m_i->first;
        }
        const String* operator-> () {
            return &(m_i->first);
        }
        const_iterator& operator++ () {
            ++m_i;
            return *this;
        }
        const_iterator operator++ (int) {
            return const_iterator (m_i++);
        }
        const_iterator& operator-- () {
            --m_i;
            return *this;
        }
        const_iterator operator-- (int) {
            return const_iterator (m_i--);
        }
        bool
        operator== (const const_iterator& i) const {
            return (m_i == i.m_i);
        }
        bool
        operator!= (const const_iterator& i) const {
            return (m_i != i.m_i);
        }

    private:
        internal_const_iterator m_i;
    };

    Config (IEventQueue* events);
    virtual ~Config ();

#ifdef TEST_ENV
    Config () : m_inputFilter (NULL) {
    }
#endif

    //! @name manipulators
    //@{

    //! Add screen
    /*!
    Adds a screen, returning true iff successful.  If a screen or
    alias with the given name exists then it fails.
    */
    bool addScreen (const String& name);

    //! Rename screen
    /*!
    Renames a screen.  All references to the name are updated.
    Returns true iff successful.
    */
    bool renameScreen (const String& oldName, const String& newName);

    //! Remove screen
    /*!
    Removes a screen.  This also removes aliases for the screen and
    disconnects any connections to the screen.  \c name may be an
    alias.
    */
    void removeScreen (const String& name);

    //! Remove all screens
    /*!
    Removes all screens, aliases, and connections.
    */
    void removeAllScreens ();

    //! Add alias
    /*!
    Adds an alias for a screen name.  An alias can be used
    any place the canonical screen name can (except addScreen()).
    Returns false if the alias name already exists or the canonical
    name is unknown, otherwise returns true.
    */
    bool addAlias (const String& canonical, const String& alias);

    //! Remove alias
    /*!
    Removes an alias for a screen name.  It returns false if the
    alias is unknown or a canonical name, otherwise returns true.
    */
    bool removeAlias (const String& alias);

    //! Remove aliases
    /*!
    Removes all aliases for a canonical screen name.  It returns false
    if the canonical name is unknown, otherwise returns true.
    */
    bool removeAliases (const String& canonical);

    //! Remove all aliases
    /*!
    This removes all aliases but not the screens.
    */
    void removeAllAliases ();

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
    bool
    connect (const String& srcName, EDirection srcSide, float srcStart,
             float srcEnd, const String& dstName, float dstStart, float dstEnd);

    //! Disconnect screens
    /*!
    Removes all connections created by connect() on side \c srcSide.
    Returns false if \c srcName is unknown.
    */
    bool disconnect (const String& srcName, EDirection srcSide);

    //! Disconnect screens
    /*!
    Removes the connections created by connect() on side \c srcSide
    covering position \c position.  Returns false if \c srcName is
    unknown.
    */
    bool disconnect (const String& srcName, EDirection srcSide, float position);

    //! Set server address
    /*!
    Set the synergy listen addresses.  There is no default address so
    this must be called to run a server using this configuration.
    */
    void setSynergyAddress (const NetworkAddress&);

    //! Add a screen option
    /*!
    Adds an option and its value to the named screen.  Replaces the
    existing option's value if there is one.  Returns true iff \c name
    is a known screen.
    */
    bool addOption (const String& name, OptionID option, OptionValue value);

    //! Remove a screen option
    /*!
    Removes an option and its value from the named screen.  Does
    nothing if the option doesn't exist on the screen.  Returns true
    iff \c name is a known screen.
    */
    bool removeOption (const String& name, OptionID option);

    //! Remove a screen options
    /*!
    Removes all options and values from the named screen.  Returns true
    iff \c name is a known screen.
    */
    bool removeOptions (const String& name);

    //! Get the hot key input filter
    /*!
    Returns the hot key input filter.  Clients can modify hotkeys using
    that object.
    */
    virtual InputFilter* getInputFilter ();

    //@}
    //! @name accessors
    //@{

    //! Test screen name validity
    /*!
    Returns true iff \c name is a valid screen name.
    */
    bool isValidScreenName (const String& name) const;

    //! Get beginning (canonical) screen name iterator
    const_iterator begin () const;
    //! Get ending (canonical) screen name iterator
    const_iterator end () const;

    //! Get beginning screen name iterator
    all_const_iterator beginAll () const;
    //! Get ending screen name iterator
    all_const_iterator endAll () const;

    //! Test for screen name
    /*!
    Returns true iff \c name names a screen.
    */
    virtual bool isScreen (const String& name) const;

    //! Test for canonical screen name
    /*!
    Returns true iff \c name is the canonical name of a screen.
    */
    bool isCanonicalName (const String& name) const;

    //! Get canonical name
    /*!
    Returns the canonical name of a screen or the empty string if
    the name is unknown.  Returns the canonical name if one is given.
    */
    String getCanonicalName (const String& name) const;

    //! Get neighbor
    /*!
    Returns the canonical screen name of the neighbor in the given
    direction (set through connect()) at position \c position.  Returns
    the empty string if there is no neighbor in that direction, otherwise
    saves the position on the neighbor in \c positionOut if it's not
    \c NULL.
    */
    String getNeighbor (const String&, EDirection, float position,
                        float* positionOut) const;

    //! Check for neighbor
    /*!
    Returns \c true if the screen has a neighbor anywhere along the edge
    given by the direction.
    */
    bool hasNeighbor (const String&, EDirection) const;

    //! Check for neighbor
    /*!
    Returns \c true if the screen has a neighbor in the given range along
    the edge given by the direction.
    */
    bool hasNeighbor (const String&, EDirection, float start, float end) const;

    //! Get beginning neighbor iterator
    link_const_iterator beginNeighbor (const String&) const;
    //! Get ending neighbor iterator
    link_const_iterator endNeighbor (const String&) const;

    //! Get the server address
    const NetworkAddress& getSynergyAddress () const;

    //! Get the screen options
    /*!
    Returns all the added options for the named screen.  Returns NULL
    if the screen is unknown and an empty collection if there are no
    options.
    */
    const ScreenOptions* getOptions (const String& name) const;

    //! Check for lock to screen action
    /*!
    Returns \c true if this configuration has a lock to screen action.
    This is for backwards compatible support of ScrollLock locking.
    */
    bool hasLockToScreenAction () const;

    //! Compare configurations
    bool operator== (const Config&) const;
    //! Compare configurations
    bool operator!= (const Config&) const;

    //! Read configuration
    /*!
    Reads a configuration from a context.  Throws XConfigRead on error
    and context is unchanged.
    */
    void read (ConfigReadContext& context);

    //! Read configuration
    /*!
    Reads a configuration from a stream.  Throws XConfigRead on error.
    */
    friend std::istream& operator>> (std::istream&, Config&);

    //! Write configuration
    /*!
    Writes a configuration to a stream.
    */
    friend std::ostream& operator<< (std::ostream&, const Config&);

    //! Get direction name
    /*!
    Returns the name of a direction (for debugging).
    */
    static const char* dirName (EDirection);

    //! Get interval as string
    /*!
    Returns an interval as a parseable string.
    */
    static String formatInterval (const Interval&);

    //@}

private:
    void readSection (ConfigReadContext&);
    void readSectionOptions (ConfigReadContext&);
    void readSectionScreens (ConfigReadContext&);
    void readSectionLinks (ConfigReadContext&);
    void readSectionAliases (ConfigReadContext&);

    InputFilter::Condition*
    parseCondition (ConfigReadContext&, const String& condition,
                    const std::vector<String>& args);
    void parseAction (ConfigReadContext&, const String& action,
                      const std::vector<String>& args, InputFilter::Rule&,
                      bool activate);

    void parseScreens (ConfigReadContext&, const String&,
                       std::set<String>& screens) const;
    static const char* getOptionName (OptionID);
    static String getOptionValue (OptionID, OptionValue);

private:
    CellMap m_map;
    NameMap m_nameToCanonicalName;
    NetworkAddress m_synergyAddress;
    ScreenOptions m_globalOptions;
    InputFilter m_inputFilter;
    bool m_hasLockToScreenAction;
    IEventQueue* m_events;
};

//! Configuration read context
/*!
Maintains a context when reading a configuration from a stream.
*/
class ConfigReadContext {
public:
    typedef std::vector<String> ArgList;

    ConfigReadContext (std::istream&, SInt32 firstLine = 1);
    ~ConfigReadContext ();

    bool readLine (String&);
    UInt32 getLineNumber () const;

    bool operator! () const;

    OptionValue parseBoolean (const String&) const;
    OptionValue parseInt (const String&) const;
    OptionValue parseModifierKey (const String&) const;
    OptionValue parseCorner (const String&) const;
    OptionValue parseCorners (const String&) const;
    Config::Interval parseInterval (const ArgList& args) const;
    void parseNameWithArgs (const String& type, const String& line,
                            const String& delim, String::size_type& index,
                            String& name, ArgList& args) const;
    IPlatformScreen::KeyInfo* parseKeystroke (const String& keystroke) const;
    IPlatformScreen::KeyInfo*
    parseKeystroke (const String& keystroke,
                    const std::set<String>& screens) const;
    IPlatformScreen::ButtonInfo* parseMouse (const String& mouse) const;
    KeyModifierMask parseModifier (const String& modifiers) const;
    std::istream&
    getStream () const {
        return m_stream;
    };

private:
    // not implemented
    ConfigReadContext& operator= (const ConfigReadContext&);

    static String concatArgs (const ArgList& args);

private:
    std::istream& m_stream;
    SInt32 m_line;
};

//! Configuration stream read exception
/*!
Thrown when a configuration stream cannot be parsed.
*/
class XConfigRead : public XBase {
public:
    XConfigRead (const ConfigReadContext& context, const String&);
    XConfigRead (const ConfigReadContext& context, const char* errorFmt,
                 const String& arg);
    virtual ~XConfigRead () _NOEXCEPT;

protected:
    // XBase overrides
    virtual String getWhat () const throw ();

private:
    String m_error;
};
