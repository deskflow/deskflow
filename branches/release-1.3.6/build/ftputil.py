from ftplib import FTP

class FtpUploader:
	def __init__(self, host, user, password, dir):
		self.host = host
		self.user = user
		self.password = password
		self.dir = dir

	def run(self, src, dest):
		
		ftp = FTP(self.host, self.user, self.password)
		ftp.cwd(self.dir)
		
		f = open(src, 'rb')
		ftp.storbinary('STOR ' + dest, f)
		f.close()
		
		ftp.close()