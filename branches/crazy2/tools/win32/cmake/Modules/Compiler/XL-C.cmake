SET(CMAKE_C_VERBOSE_FLAG "-V")

# -qthreaded     = Ensures that all optimizations will be thread-safe
# -qalias=noansi = Turns off type-based aliasing completely (safer optimizer)
# -qhalt=e       = Halt on error messages (rather than just severe errors)
SET(CMAKE_C_FLAGS_INIT "-qthreaded -qalias=noansi -qhalt=e")

SET(CMAKE_C_FLAGS_DEBUG_INIT "-g")
SET(CMAKE_C_FLAGS_RELEASE_INIT "-O -DNDEBUG")
SET(CMAKE_C_FLAGS_MINSIZEREL_INIT "-O -DNDEBUG")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-g")

SET(CMAKE_C_CREATE_PREPROCESSED_SOURCE "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -E <SOURCE> > <PREPROCESSED_SOURCE>")
SET(CMAKE_C_CREATE_ASSEMBLY_SOURCE "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -S <SOURCE> -o <ASSEMBLY_SOURCE>")
