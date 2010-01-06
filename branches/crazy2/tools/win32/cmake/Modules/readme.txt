For more information about how to contribute modules to CMake, see this page:
http://www.itk.org/Wiki/CMake:Module_Maintainers

Note to authors of FindXXX.cmake files

We would like all FindXXX.cmake files to produce consistent variable names.

Please use the following consistent variable names for general use.

XXX_INCLUDE_DIRS        The final set of include directories listed in one variable for use by client code.  This should not be a cache entry.
XXX_LIBRARIES          	The libraries to link against to use XXX. These should include full paths.  This should not be a cache entry.
XXX_DEFINITIONS        	Definitions to use when compiling code that uses XXX. This really shouldn't include options such as (-DHAS_JPEG)that a client source-code file uses to decide whether to #include <jpeg.h>
XXX_EXECUTABLE         	Where to find the XXX tool.
XXX_YYY_EXECUTABLE     	Where to find the YYY tool that comes with XXX.
XXX_LIBRARY_DIRS        Optionally, the final set of library directories listed in one variable for use by client code.  This should not be a cache entry.
XXX_ROOT_DIR           	Where to find the base directory of XXX.
XXX_VERSION_YY		Expect Version YY if true. Make sure at most one of these is ever true.
XXX_WRAP_YY		If False, do not try to use the relevent CMake wrapping command.
XXX_YY_FOUND           	If False, optional YY part of XXX sytem is not available.
XXX_FOUND              	Set to false, or undefined, if we haven't found, or don't want to use XXX.
XXX_RUNTIME_LIBRARY_DIRS Optionally, the runtime library search path for use when running an executable linked to shared libraries.
                         The list should be used by user code to create the PATH on windows or LD_LIBRARY_PATH on unix.
                         This should not be a cache entry.
XXX_VERSION_STRING      A human-readable string containing the version of the package found, if any.
XXX_VERSION_MAJOR       The major version of the package found, if any.
XXX_VERSION_MINOR       The minor version of the package found, if any.
XXX_VERSION_PATCH       The patch version of the package found, if any.

You do not have to provide all of the above variables. You should provide XXX_FOUND under most circumstances. If XXX is a library, then  XXX_LIBRARIES, should also be defined, and XXX_INCLUDE_DIRS should usually be defined (I guess libm.a might be an exception)

The following names should not usually be used in CMakeLists.txt files, but they may be usefully modified in users' CMake Caches to control stuff.

XXX_LIBRARY		Name of XXX Library. A User may set this and XXX_INCLUDE_DIR to ignore to force non-use of XXX.
XXX_YY_LIBRARY		Name of YY library that is part of the XXX system. It may or may not be required to use XXX.
XXX_INCLUDE_DIR        	Where to find xxx.h, etc.  (XXX_INCLUDE_PATH was considered bad because a path includes an actual filename.)
XXX_YY_INCLUDE_DIR      Where to find xxx_yy.h, etc.

For tidiness's sake, try to keep as many options as possible out of the cache, leaving at least one option which can be used to disable use of the module, or locate a not-found library (e.g. XXX_ROOT_DIR). For the same reason, mark most cache options as advanced.

If you need other commands to do special things then it should still begin with XXX_. This gives a sort of namespace effect and keeps things tidy for the user. You should put comments describing all the exported settings, plus descriptions of any the users can use to control stuff.

You really should also provide backwards compatibility any old settings that were actually in use. Make sure you comment them as deprecated, so that no-one starts using them.

To correctly document a module, create a comment block at the top with # comments.  There are three types of comments that can be in the block:

1. The brief description of the module, this is done by:
# - a small description

2. A paragraph of text.  This is done with all text that has a single
space between the # and the text.  To create a new paragraph, just
put a # with no text on the line.

3. A verbatim line.  This is done with two spaces between the # and the text.

For example:

# - This is a cool module
# This module does really cool stuff.
# It can do even more than you think.
# 
# It even needs to paragraphs to tell you about it.
# And it defines the following variables:
#  VAR_COOL - this is great isn't it?
#  VAR_REALLY_COOL - cool right?
#

To have a .cmake file in this directory NOT show up in the
modules documentation, you should start the file with a blank
line.

A FindXXX.cmake module will typically be loaded by the command

  FIND_PACKAGE(XXX [major[.minor[.patch[.tweak]]]] [EXACT]
               [QUIET] [[REQUIRED|COMPONENTS] [components...]])

If any version numbers are given to the command it will set the
following variables before loading the module:

  XXX_FIND_VERSION       = full requested version string
  XXX_FIND_VERSION_MAJOR = major version if requested, else 0
  XXX_FIND_VERSION_MINOR = minor version if requested, else 0
  XXX_FIND_VERSION_PATCH = patch version if requested, else 0
  XXX_FIND_VERSION_TWEAK = tweak version if requested, else 0
  XXX_FIND_VERSION_COUNT = number of version components, 0 to 4
  XXX_FIND_VERSION_EXACT = true if EXACT option was given

If the find module supports versioning it should locate a version of
the package that is compatible with the version requested.  If a
compatible version of the package cannot be found the module should
not report success.  The version of the package found should be stored
in "XXX_VERSION..." version variables documented by the module.

If the QUIET option is given to the command it will set the variable
XXX_FIND_QUIETLY to true before loading the FindXXX.cmake module.  If
this variable is set the module should not complain about not being
able to find the package.  If the
REQUIRED option is given to the command it will set the variable
XXX_FIND_REQUIRED to true before loading the FindXXX.cmake module.  If
this variable is set the module should issue a FATAL_ERROR if the
package cannot be found.  For each package-specific component, say
YYY, listed after the REQUIRED option a variable XXX_FIND_REQUIRED_YYY
to true.  The set of components listed after either the REQUIRED
option or the COMPONENTS option will be specified in a
XXX_FIND_COMPONENTS variable.  This can be used by the FindXXX.cmake
module to determine which sub-components of the package must be found.
If neither the QUIET nor REQUIRED options are given then the
FindXXX.cmake module should look for the package and complain without
error if the module is not found.

To get this behaviour you can use the FIND_PACKAGE_HANDLE_STANDARD_ARGS() 
macro, as an example see FindJPEG.cmake.

For internal implementation, it's a generally accepted convention that variables starting with
underscore are for temporary use only. (variable starting with an underscore
are not intended as a reserved prefix).
