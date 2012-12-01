# synergy-web -- website for synergy
# Copyright (C) 2012 Bolton Software Ltd.
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys, re, ConfigParser, io
import MySQLdb as mysql
from getopt import gnu_getopt

def runHelp():
  print ("-h      help\n"
         "-p      fix payments tables")

def runGeneratePayments():
  config = ConfigParser.RawConfigParser()
  config.read("settings.ini")
  
  host = config.get("database", "host")
  user = config.get("database", "user")
  password = config.get("database", "pass")
  name = config.get("database", "name")
  
  con = mysql.connect(host, user, password, name);
  cur = con.cursor()
  cur.execute("select id, ipnData from paypal")
  
  print "getting paypal payments..."
  paypal = []
  for id, ipnData in cur:
    paymentMatch = re.search("\"payment_gross\"\:\"(\-?\d+\.\d+)\"", ipnData)
    if (paymentMatch):
      amount = float(paymentMatch.group(1))
      print str(id) + ": amount=" + str(amount)
      paypal.append((id, amount))
    else:
      print str(id) + ": no amount"
  
  print "updating paypal table..."
  for id, amount in paypal:
    sql = "update paypal set amount = %f where id = %d" % (amount, id)
    print sql
    cur = con.cursor()
    cur.execute(sql)
  
  cur.execute("commit")

if len(sys.argv) == 1:
	print "usage: tools [-h|-p]"
	sys.exit()

opts, args = gnu_getopt(sys.argv, "hp", "")
for o, a in opts:
	if o == "-h":
		runHelp()
	elif o == "-p":
		runGeneratePayments()
