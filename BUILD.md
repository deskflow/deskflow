# Build Synergy

## Developer Quick Start

Synergy 1 Community Edition is free and open source software, and anyone is welcome to build it,
run it, tinker with it, redistribute it as part of their own app, etc.

These instructions will build Synergy 1 Community Edition, which doesn't require a license
or serial key. Check the [Developer Guide](https://github.com/symless/synergy/wiki/Developer-Guide)
wiki page if you have problems.

**1. Dependencies:**

*macOS, Linux, or BSD-derived:*
```
./scripts/install_deps.sh
```

*Windows:*
```
python scripts/install_deps.py
```

**2. Configure:**

*Linux:*
```
cmake -B build --preset=linux-release
```

*macOS:*
```
cmake -B build --preset=macos-release
```

*Windows:*
```
cmake -B build --preset=windows-release
```

**3. Build:**
```
cmake --build build -j8
```

**4. Test:**
```
./build/bin/unittests
./build/bin/integtests
```

**5. Run**
```
./build/bin/synergy
```
