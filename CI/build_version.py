import subprocess

class VersionPart:

   def __init__(self, part = ''):
      self.prefix = ''
      self.suffix = ''
      self.number = 0

      if part:
         self.__parsePreffix(part)
         self.__parseNumber(part)
         self.__parseSuffix(part)

   def __parsePreffix(self, part):
      if not part[0].isdigit():
         for i in part:
            if not i.isdigit():
               self.prefix += i
            else:
               break

   def __parseNumber(self, part):
      start = len(self.prefix)
      end = part.find('-')
      if end > 0:
         self.number = int(part[start:end])
      else:
         self.number = int(part[start:])

   def __parseSuffix(self, part):
      items = part.split('-')
      if len(items) == 2:
         self.suffix = '-' + items[1]

   def __str__(self):
      return self.prefix + str(self.number) + self.suffix

class Version:

   def __init__(self, version):
      versionParts = version.split('.')

      if len(versionParts) == 3:
         self.major = VersionPart(versionParts[0])
         self.minor = VersionPart(versionParts[1])
         self.build = VersionPart(versionParts[2])
         self.patch = VersionPart()
         self.patch.number = self.build.number
      elif len(versionParts) == 4:
         self.major = VersionPart(versionParts[0])
         self.minor = VersionPart(versionParts[1])
         self.patch = VersionPart(versionParts[2])
         self.build = VersionPart(versionParts[3])
      else:
         print('ERROR: Wrong version number')

   def __str__(self):
      result  = str(self.major) + '.'
      result += str(self.minor) + '.'
      result += str(self.patch) + '.'
      result += str(self.build) 
      return  result

   def nextBuild(self):
      self.build.number += 1

class VersionFile:
   def __init__(self, file):
      self.file = file

   def setOption(self, name, value):
      fp = open(self.file, 'rt+')
      content = ''
      for line in fp:
         print(line)
         if line.find(name) != -1:
            line = 'set ('+ name + ' ' + value + ')'
         content += line
      fp.write(content)
      fp.close()


def getVesionFromGit():
   cmd = ('git tag --sort=-creatordate').split()

   try:
      proccess = subprocess.run(cmd, stdout=subprocess.PIPE, text=True)
      versions = proccess.stdout.split()
      print('INFO: Version '+ versions[0] + ' has been read from git')
      return versions[0]
   except subprocess.CalledProcessError:
      print('ERROR: Unable to get version from git')
      exit(1)

def updateVersionFile(number):
   fp = open('cmake/Version.cmake')
   content = fp.read()
   fp.close()

   fp = open('cmake/Version.cmake', 'wt')
   fp.write(content.replace('set (SYNERGY_VERSION_BUILD 1)', 'set (SYNERGY_VERSION_BUILD ' + str(number) + ')'))
   fp.close()

if __name__ == '__main__':
   version = Version(getVesionFromGit())
   version.nextBuild()
   updateVersionFile(version.build.number)
   print('INFO: Generate build number is: <' + str(version) + '>')