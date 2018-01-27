Mac OS X Readme
===============

To install on Mac OS X with the .zip distribution (first seen in 1.3.6) you must follow these steps:

  1. Extract the zip file to any location (usually double click will do this)
  2. Open Terminal, and cd to the extracted directory (e.g. /Users/my-name/Downloads/extracted-dir/)
  3. Copy the binaries to /usr/bin using: sudo cp barrier* /usr/bin
  4. Correct the permissions and ownership: sudo chown root:wheel /usr/bin/barrier*; sudo chmod 555 /usr/bin/barrier*

Alternatively, you can copy the binaries as root. How to enable the root user in Mac OS X:
  http://support.apple.com/en-us/ht1528

Once the binaries have been copied to /usr/bin, you should follow the configuration guide:
  http://synergy2.sourceforge.net/configuration.html
