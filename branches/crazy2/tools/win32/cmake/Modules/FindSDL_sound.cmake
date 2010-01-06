# Locates the SDL_sound library

# This module depends on SDL being found and 
# must be called AFTER FindSDL.cmake is called.

# This module defines
# SDL_SOUND_INCLUDE_DIR, where to find SDL_sound.h
# SDL_SOUND_FOUND, if false, do not try to link to SDL
# SDL_SOUND_LIBRARIES, this contains the list of libraries that you need 
# to link against. This is a read-only variable and is marked INTERNAL.
# SDL_SOUND_EXTRAS, this is an optional variable for you to add your own
# flags to SDL_SOUND_LIBRARIES. This is prepended to SDL_SOUND_LIBRARIES.
# This is available mostly for cases this module failed to anticipate for
# and you must add additional flags. This is marked as ADVANCED.
 
#
# This module also defines (but you shouldn't need to use directly)
# SDL_SOUND_LIBRARY, the name of just the SDL_sound library you would link
# against. Use SDL_SOUND_LIBRARIES for you link instructions and not this one.
# And might define the following as needed
# MIKMOD_LIBRARY
# MODPLUG_LIBRARY
# OGG_LIBRARY
# VORBIS_LIBRARY
# SMPEG_LIBRARY
# FLAC_LIBRARY
# SPEEX_LIBRARY
#
# Typically, you should not use these variables directly, and you should use 
# SDL_SOUND_LIBRARIES which contains SDL_SOUND_LIBRARY and the other audio libraries 
# (if needed) to successfully compile on your system . 
#
# Created by Eric Wing. 
# This module is a bit more complicated than the other FindSDL* family modules.
# The reason is that SDL_sound can be compiled in a large variety of different ways
# which are independent of platform. SDL_sound may dynamically link against other 3rd
# party libraries to get additional codec support, such as Ogg Vorbis, SMPEG, ModPlug,
# MikMod, FLAC, Speex, and potentially others. 
# Under some circumstances which I don't fully understand, 
# there seems to be a requirement
# that dependent libraries of libraries you use must also be explicitly 
# linked against in order to successfully compile. SDL_sound does not currently 
# have any system in place to know how it was compiled.
# So this CMake module does the hard work in trying to discover which 3rd party 
# libraries are required for building (if any).
# This module uses a brute force approach to create a test program that uses SDL_sound,
# and then tries to build it. If the build fails, it parses the error output for 
# known symbol names to figure out which libraries are needed.
#
# Responds to the $SDLDIR and $SDLSOUNDDIR environmental variable that would
# correspond to the ./configure --prefix=$SDLDIR used in building SDL.
#
# On OSX, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of 
# SDL_LIBRARY to override this selectionor set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.
#

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

SET(SDL_SOUND_EXTRAS "" CACHE STRING "SDL_sound extra flags")
MARK_AS_ADVANCED(SDL_SOUND_EXTRAS)

# Find SDL_sound.h
FIND_PATH(SDL_SOUND_INCLUDE_DIR SDL_sound.h
  HINTS
  $ENV{SDLSOUNDDIR}/include
  $ENV{SDLSOUNDDIR}
  $ENV{SDLDIR}/include
  $ENV{SDLDIR}
  PATHS
  /usr/local/include/SDL
  /usr/include/SDL
  /usr/local/include/SDL12
  /usr/local/include/SDL11 # FreeBSD ports
  /usr/include/SDL12
  /usr/include/SDL11
  /usr/local/include
  /usr/include
  /sw/include/SDL # Fink
  /sw/include
  /opt/local/include/SDL # DarwinPorts
  /opt/local/include
  /opt/csw/include/SDL # Blastwave
  /opt/csw/include 
  /opt/include/SDL
  /opt/include
  )

FIND_LIBRARY(SDL_SOUND_LIBRARY 
  NAMES SDL_sound
  HINTS
  $ENV{SDLSOUNDDIR}/lib
  $ENV{SDLSOUNDDIR}
  $ENV{SDLDIR}/lib
  $ENV{SDLDIR}
  PATHS
  /usr/local/lib
  /usr/lib
  /sw/lib
  /opt/local/lib
  /opt/csw/lib
  /opt/lib
  )

SET(SDL_SOUND_FOUND "NO")
IF(SDL_FOUND AND SDL_SOUND_INCLUDE_DIR AND SDL_SOUND_LIBRARY)

  # CMake is giving me problems using TRY_COMPILE with the CMAKE_FLAGS
  # for the :STRING syntax if I have multiple values contained in a
  # single variable. This is a problem for the SDL_LIBRARY variable
  # because it does just that. When I feed this variable to the command,
  # only the first value gets the appropriate modifier (e.g. -I) and 
  # the rest get dropped.
  # To get multiple single variables to work, I must separate them with a "\;"
  # I could go back and modify the FindSDL.cmake module, but that's kind of painful.
  # The solution would be to try something like:
  # SET(SDL_TRY_COMPILE_LIBRARY_LIST "${SDL_TRY_COMPILE_LIBRARY_LIST}\;${CMAKE_THREAD_LIBS_INIT}")
  # Instead, it was suggested on the mailing list to write a temporary CMakeLists.txt
  # with a temporary test project and invoke that with TRY_COMPILE.
  # See message thread "Figuring out dependencies for a library in order to build"
  # 2005-07-16  
  #     TRY_COMPILE( 
  #             MY_RESULT
  #             ${CMAKE_BINARY_DIR}
  #             ${PROJECT_SOURCE_DIR}/DetermineSoundLibs.c
  #             CMAKE_FLAGS 
  #                     -DINCLUDE_DIRECTORIES:STRING=${SDL_INCLUDE_DIR}\;${SDL_SOUND_INCLUDE_DIR}
  #                     -DLINK_LIBRARIES:STRING=${SDL_SOUND_LIBRARY}\;${SDL_LIBRARY}
  #             OUTPUT_VARIABLE MY_OUTPUT
  #     )

  # To minimize external dependencies, create a sdlsound test program
  # which will be used to figure out if additional link dependencies are
  # required for the link phase.
  FILE(WRITE ${PROJECT_BINARY_DIR}/CMakeTmp/DetermineSoundLibs.c
    "#include \"SDL_sound.h\"
    #include \"SDL.h\"
    int main(int argc, char* argv[])
    {
        Sound_AudioInfo desired;
        Sound_Sample* sample;

        SDL_Init(0);
        Sound_Init();
        
        /* This doesn't actually have to work, but Init() is a no-op
         * for some of the decoders, so this should force more symbols
         * to be pulled in.
         */
        sample = Sound_NewSampleFromFile(argv[1], &desired, 4096);
        
        Sound_Quit();
        SDL_Quit();
        return 0;
     }"
     )

   # Calling 
   # TARGET_LINK_LIBRARIES(DetermineSoundLibs "${SDL_SOUND_LIBRARY} ${SDL_LIBRARY})
   # causes problems when SDL_LIBRARY looks like
   # /Library/Frameworks/SDL.framework;-framework Cocoa
   # The ;-framework Cocoa seems to be confusing CMake once the OS X
   # framework support was added. I was told that breaking up the list 
   # would fix the problem.
   SET(TMP_TRY_LIBS)
   FOREACH(lib ${SDL_SOUND_LIBRARY} ${SDL_LIBRARY})
     SET(TMP_TRY_LIBS "${TMP_TRY_LIBS} \"${lib}\"")
   ENDFOREACH(lib)

   # MESSAGE("TMP_TRY_LIBS ${TMP_TRY_LIBS}")
   
   # Write the CMakeLists.txt and test project
   # Weird, this is still sketchy. If I don't quote the variables
   # in the TARGET_LINK_LIBRARIES, I seem to loose everything 
   # in the SDL_LIBRARY string after the "-framework".
   # But if I quote the stuff in INCLUDE_DIRECTORIES, it doesn't work.
   FILE(WRITE ${PROJECT_BINARY_DIR}/CMakeTmp/CMakeLists.txt
     "PROJECT(DetermineSoundLibs)
        INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR} ${SDL_SOUND_INCLUDE_DIR})
        ADD_EXECUTABLE(DetermineSoundLibs DetermineSoundLibs.c)
        TARGET_LINK_LIBRARIES(DetermineSoundLibs ${TMP_TRY_LIBS})"
     )

   TRY_COMPILE( 
     MY_RESULT
     ${PROJECT_BINARY_DIR}/CMakeTmp
     ${PROJECT_BINARY_DIR}/CMakeTmp
     DetermineSoundLibs
     OUTPUT_VARIABLE MY_OUTPUT
     )
   
   # MESSAGE("${MY_RESULT}")
   # MESSAGE(${MY_OUTPUT})
   
   IF(NOT MY_RESULT)
     
     # I expect that MPGLIB, VOC, WAV, AIFF, and SHN are compiled in statically.
     # I think Timidity is also compiled in statically.
     # I've never had to explcitly link against Quicktime, so I'll skip that for now.
     
     SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARY})
     
     # Find MikMod
     IF("${MY_OUTPUT}" MATCHES "MikMod_")
     FIND_LIBRARY(MIKMOD_LIBRARY
         NAMES libmikmod-coreaudio mikmod
         PATHS
         $ENV{MIKMODDIR}/lib
         $ENV{MIKMODDIR}
         $ENV{SDLSOUNDDIR}/lib
         $ENV{SDLSOUNDDIR}
         $ENV{SDLDIR}/lib
         $ENV{SDLDIR}
         /usr/local/lib
         /usr/lib
         /sw/lib
         /opt/local/lib
         /opt/csw/lib
       /opt/lib
       ) 
       IF(MIKMOD_LIBRARY)
         SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARIES_TMP} ${MIKMOD_LIBRARY})
       ENDIF(MIKMOD_LIBRARY)
     ENDIF("${MY_OUTPUT}" MATCHES "MikMod_")
     
     # Find ModPlug
     IF("${MY_OUTPUT}" MATCHES "MODPLUG_")
       FIND_LIBRARY(MODPLUG_LIBRARY
         NAMES modplug
         PATHS
         $ENV{MODPLUGDIR}/lib
         $ENV{MODPLUGDIR}
         $ENV{SDLSOUNDDIR}/lib
         $ENV{SDLSOUNDDIR}
         $ENV{SDLDIR}/lib
         $ENV{SDLDIR}
         /usr/local/lib
         /usr/lib
         /sw/lib
         /opt/local/lib
         /opt/csw/lib
       /opt/lib
       )
       IF(MODPLUG_LIBRARY)
         SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARIES_TMP} ${MODPLUG_LIBRARY})
       ENDIF(MODPLUG_LIBRARY)
     ENDIF("${MY_OUTPUT}" MATCHES "MODPLUG_")

     
     # Find Ogg and Vorbis
     IF("${MY_OUTPUT}" MATCHES "ov_")
       FIND_LIBRARY(VORBIS_LIBRARY
         NAMES vorbis Vorbis VORBIS
         PATHS
         $ENV{VORBISDIR}/lib
         $ENV{VORBISDIR}
         $ENV{OGGDIR}/lib
         $ENV{OGGDIR}
         $ENV{SDLSOUNDDIR}/lib
         $ENV{SDLSOUNDDIR}
         $ENV{SDLDIR}/lib
         $ENV{SDLDIR}
         /usr/local/lib
         /usr/lib
         /sw/lib
         /opt/local/lib
         /opt/csw/lib
       /opt/lib
         )
       IF(VORBIS_LIBRARY)
         SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARIES_TMP} ${VORBIS_LIBRARY})
       ENDIF(VORBIS_LIBRARY)
       
       FIND_LIBRARY(OGG_LIBRARY
         NAMES ogg Ogg OGG
         PATHS
         $ENV{OGGDIR}/lib
         $ENV{OGGDIR}
         $ENV{VORBISDIR}/lib
         $ENV{VORBISDIR}
         $ENV{SDLSOUNDDIR}/lib
         $ENV{SDLSOUNDDIR}
         $ENV{SDLDIR}/lib
         $ENV{SDLDIR}
         /usr/local/lib
         /usr/lib
         /sw/lib
         /opt/local/lib
         /opt/csw/lib
       /opt/lib
         )
       IF(OGG_LIBRARY)
         SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARIES_TMP} ${OGG_LIBRARY})
       ENDIF(OGG_LIBRARY)
     ENDIF("${MY_OUTPUT}" MATCHES "ov_")
     
     
     # Find SMPEG
     IF("${MY_OUTPUT}" MATCHES "SMPEG_")
       FIND_LIBRARY(SMPEG_LIBRARY
         NAMES smpeg SMPEG Smpeg SMpeg
         PATHS
         $ENV{SMPEGDIR}/lib
         $ENV{SMPEGDIR}
         $ENV{SDLSOUNDDIR}/lib
         $ENV{SDLSOUNDDIR}
         $ENV{SDLDIR}/lib
         $ENV{SDLDIR}
         /usr/local/lib
         /usr/lib
         /sw/lib
         /opt/local/lib
         /opt/csw/lib
       /opt/lib
         )
       IF(SMPEG_LIBRARY)
         SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARIES_TMP} ${SMPEG_LIBRARY})
       ENDIF(SMPEG_LIBRARY)
     ENDIF("${MY_OUTPUT}" MATCHES "SMPEG_")
     
     
     # Find FLAC
     IF("${MY_OUTPUT}" MATCHES "FLAC_")
       FIND_LIBRARY(FLAC_LIBRARY
         NAMES flac FLAC
         PATHS
         $ENV{FLACDIR}/lib
         $ENV{FLACDIR}
         $ENV{SDLSOUNDDIR}/lib
         $ENV{SDLSOUNDDIR}
         $ENV{SDLDIR}/lib
         $ENV{SDLDIR}
         /usr/local/lib
         /usr/lib
         /sw/lib
         /opt/local/lib
         /opt/csw/lib
       /opt/lib
         )
       IF(FLAC_LIBRARY)
         SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARIES_TMP} ${FLAC_LIBRARY})
       ENDIF(FLAC_LIBRARY)
     ENDIF("${MY_OUTPUT}" MATCHES "FLAC_")
     
     
     # Hmmm...Speex seems to depend on Ogg. This might be a problem if
     # the TRY_COMPILE attempt gets blocked at SPEEX before it can pull
     # in the Ogg symbols. I'm not sure if I should duplicate the ogg stuff
     # above for here or if two ogg entries will screw up things.
     IF("${MY_OUTPUT}" MATCHES "speex_")
       FIND_LIBRARY(SPEEX_LIBRARY
         NAMES speex SPEEX
         PATHS
         $ENV{SPEEXDIR}/lib
         $ENV{SPEEXDIR}
         $ENV{SDLSOUNDDIR}/lib
         $ENV{SDLSOUNDDIR}
         $ENV{SDLDIR}/lib
         $ENV{SDLDIR}
         /usr/local/lib
         /usr/lib
         /sw/lib
         /opt/local/lib
         /opt/csw/lib
       /opt/lib
         )
       IF(SPEEX_LIBRARY)
         SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARIES_TMP} ${SPEEX_LIBRARY})
       ENDIF(SPEEX_LIBRARY)
       
       # Find OGG (needed for Speex)
     # We might have already found Ogg for Vorbis, so skip it if so.
       IF(NOT OGG_LIBRARY)
         FIND_LIBRARY(OGG_LIBRARY
           NAMES ogg Ogg OGG
           PATHS
           $ENV{OGGDIR}/lib
           $ENV{OGGDIR}
           $ENV{VORBISDIR}/lib
           $ENV{VORBISDIR}
           $ENV{SPEEXDIR}/lib
           $ENV{SPEEXDIR}
           $ENV{SDLSOUNDDIR}/lib
           $ENV{SDLSOUNDDIR}
           $ENV{SDLDIR}/lib
           $ENV{SDLDIR}
           /usr/local/lib
           /usr/lib
           /sw/lib
           /opt/local/lib
           /opt/csw/lib
         /opt/lib
           )
         IF(OGG_LIBRARY)
           SET(SDL_SOUND_LIBRARIES_TMP ${SDL_SOUND_LIBRARIES_TMP} ${OGG_LIBRARY})
         ENDIF(OGG_LIBRARY)
       ENDIF(NOT OGG_LIBRARY)
     ENDIF("${MY_OUTPUT}" MATCHES "speex_")
     
   ELSE(NOT MY_RESULT)
     SET(SDL_SOUND_LIBRARIES "${SDL_SOUND_EXTRAS} ${SDL_SOUND_LIBRARY}" CACHE INTERNAL "SDL_sound and dependent libraries")
   ENDIF(NOT MY_RESULT)

   SET(SDL_SOUND_LIBRARIES "${SDL_SOUND_EXTRAS} ${SDL_SOUND_LIBRARIES_TMP}" CACHE INTERNAL "SDL_sound and dependent libraries")
   SET(SDL_SOUND_FOUND "YES")
 ENDIF(SDL_FOUND AND SDL_SOUND_INCLUDE_DIR AND SDL_SOUND_LIBRARY)

 # MESSAGE("SDL_SOUND_LIBRARIES is ${SDL_SOUND_LIBRARIES}")

