/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- Configuration.h
//
// This header defines a set of classes used to represent configuration
// parameters of different types.  Instances of the different parameter
// types are associated with instances of the Configuration class, and
// are each given a unique name.  The Configuration class provides a
// generic API through which parameters may be located by name and their
// value set, thus removing the need to write platform-specific code.
// Simply defining a new parameter and associating it with a Configuration
// will allow it to be configured by the user.
//
// If no Configuration is specified when creating a Parameter, then the
// global Configuration will be assumed.
//
// Configurations can be "chained" into groups.  Each group has a root
// Configuration, a pointer to which should be passed to the constructors
// of the other group members.  set() and get() operations called on the
// root will iterate through all of the group's members.
//
// NB: On platforms that support Threading, locking is performed to protect
//     complex parameter types from concurrent access (e.g. strings).
// NB: NO LOCKING is performed when linking Configurations to groups
//     or when adding Parameters to Configurations.

#ifndef __RFB_CONFIGURATION_H__
#define __RFB_CONFIGURATION_H__

#include <rfb/util.h>

namespace rfb {
  class VoidParameter;
  struct ParameterIterator;

  // -=- Configuration
  //     Class used to access parameters.

  class Configuration {
  public:
    // - Create a new Configuration object
    Configuration(const char* name, Configuration* attachToGroup=0);

    // - Return the buffer containing the Configuration's name
    const char* getName() const { return name.buf; }

    // - Assignment operator.  For every Parameter in this Configuration's
    //   group, get()s the corresponding source parameter and copies its
    //   content.
    Configuration& operator=(const Configuration& src);

    // - Set named parameter to value
    bool set(const char* param, const char* value, bool immutable=false);

    // - Set parameter to value (separated by "=")
    bool set(const char* config, bool immutable=false);

    // - Set named parameter to value, with name truncated at len
    bool set(const char* name, int len,
                  const char* val, bool immutable);

    // - Get named parameter
    VoidParameter* get(const char* param);

    // - List the parameters of this Configuration group
    void list(int width=79, int nameWidth=10);

    // - readFromFile
    //   Read configuration parameters from the specified file.
    void readFromFile(const char* filename);

    // - writeConfigToFile
    //   Write a new configuration parameters file, then mv it
    //   over the old file.
    void writeToFile(const char* filename);


    // - Get the Global Configuration object
    //   NB: This call does NOT lock the Configuration system.
    //       ALWAYS ensure that if you have ANY global Parameters,
    //       then they are defined as global objects, to ensure that
    //       global() is called when only the main thread is running.
    static Configuration* global();

    // - Container for process-wide Global parameters
    static bool setParam(const char* param, const char* value, bool immutable=false) {
      return global()->set(param, value, immutable);
    }
    static bool setParam(const char* config, bool immutable=false) { 
      return global()->set(config, immutable);
    }
    static bool setParam(const char* name, int len,
      const char* val, bool immutable) {
      return global()->set(name, len, val, immutable);
    }
    static VoidParameter* getParam(const char* param) { return global()->get(param); }
    static void listParams(int width=79, int nameWidth=10) { global()->list(width, nameWidth); }

  protected:
    friend class VoidParameter;
    friend struct ParameterIterator;

    // Name for this Configuration
    CharArray name;

    // - Pointer to first Parameter in this group
    VoidParameter* head;

    // Pointer to next Configuration in this group
    Configuration* _next;

    // The process-wide, Global Configuration object
    static Configuration* global_;
  };

  // -=- VoidParameter
  //     Configuration parameter base-class.

  class VoidParameter {
  public:
    VoidParameter(const char* name_, const char* desc_, Configuration* conf=0);
    virtual  ~VoidParameter();
    const char* getName() const;
    const char* getDescription() const;

    virtual bool setParam(const char* value)  = 0;
    virtual bool setParam();
    virtual char* getDefaultStr() const = 0;
    virtual char* getValueStr() const = 0;
    virtual bool isBool() const;

    virtual void setImmutable();
  protected:
    friend class Configuration;
    friend struct ParameterIterator;

    VoidParameter* _next;
    bool immutable;
    const char* name;
    const char* description;
  };

  class AliasParameter : public VoidParameter {
  public:
    AliasParameter(const char* name_, const char* desc_,VoidParameter* param_, Configuration* conf=0);
    virtual bool setParam(const char* value);
    virtual bool setParam();
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;
    virtual bool isBool() const;
  private:
    VoidParameter* param;
  };

  class BoolParameter : public VoidParameter {
  public:
    BoolParameter(const char* name_, const char* desc_, bool v, Configuration* conf=0);
    virtual bool setParam(const char* value);
    virtual bool setParam();
    virtual void setParam(bool b);
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;
    virtual bool isBool() const;
    operator bool() const;
  protected:
    bool value;
    bool def_value;
  };

  class IntParameter : public VoidParameter {
  public:
    IntParameter(const char* name_, const char* desc_, int v,
                 int minValue=INT_MIN, int maxValue=INT_MAX, Configuration* conf=0);
    virtual bool setParam(const char* value);
    virtual bool setParam(int v);
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;
    operator int() const;
  protected:
    int value;
    int def_value;
    int minValue, maxValue;
  };

  class StringParameter : public VoidParameter {
  public:
    // StringParameter contains a null-terminated string, which CANNOT
    // be Null, and so neither can the default value!
    StringParameter(const char* name_, const char* desc_, const char* v, Configuration* conf=0);
    virtual ~StringParameter();
    virtual bool setParam(const char* value);
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;

    // getData() returns a copy of the data - it must be delete[]d by the
    // caller.
    char* getData() const { return getValueStr(); }
  protected:
    char* value;
    const char* def_value;
  };

  class BinaryParameter : public VoidParameter {
  public:
    BinaryParameter(const char* name_, const char* desc_, const void* v, int l, Configuration* conf=0);
    virtual ~BinaryParameter();
    virtual bool setParam(const char* value);
    virtual void setParam(const void* v, int l);
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;

    // getData() will return length zero if there is no data
    // NB: data may be set to zero, OR set to a zero-length buffer
    void getData(void** data, int* length) const;

  protected:
    char* value;
    int length;
    char* def_value;
    int def_length;
  };

  // -=- ParameterIterator
  //     Iterates over all the Parameters in a Configuration group.  The
  //     current Parameter is accessed via param, the current Configuration
  //     via config.  The next() method moves on to the next Parameter.

  struct ParameterIterator {
    ParameterIterator(Configuration* c) : config(c), param(c ? c->head : 0) {}
    void next() {
      param = param->_next;
      while (!param) {
        config = config->_next;
        if (!config) break;
        param = config->head;
      }
    }
    Configuration* config;
    VoidParameter* param;
  };

};

#endif // __RFB_CONFIGURATION_H__
