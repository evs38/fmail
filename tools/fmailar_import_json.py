#!/usr/bin/python

from fmailar import *

import json
import sys


#------------------------------------------------------------------------------
# main

fmj = open(sys.argv[1], "rb")
fmacObject = json.load(fmj)
fmj.close()

fmailAreasConfig2 = FmailAreasConfig()
fmailAreasConfig2.Load(fmacObject)

fmarw = open("fmail.new.ar", "wb")
fmailAreasConfig2.Write(fmarw)
fmarw.close()
