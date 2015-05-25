  def getDebianArch(self):
    if os.uname()[4][:3] == 'arm':
      return 'armhf'

                # os_bits should be loaded with '32bit' or '64bit'
                import platform
                (os_bits, other) = platform.architecture()
    
                # get platform based on current platform
                if os_bits == '32bit':
                        return 'i386'
                elif os_bits == '64bit':
                        return 'amd64'
                else:
                        raise Exception("unknown os bits: " + os_bits)

        def getLinuxPlatform(self):
    if os.uname()[4][:3] == 'arm':
      return 'Linux-armv6l'

                # os_bits should be loaded with '32bit' or '64bit'
                import platform
                (os_bits, other) = platform.architecture()
    
                # get platform based on current platform
                if os_bits == '32bit':
                        return 'Linux-i686'
                elif os_bits == '64bit':
                        return 'Linux-x86_64'
                else:
                        raise Exception("unknown os bits: " + os_bits)