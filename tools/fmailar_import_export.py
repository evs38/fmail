#!/usr/bin/python

from fmailar import *

import json


#------------------------------------------------------------------------------
# main
fmailAreasConfig = FmailAreasConfig()

fmar = open(FN_FMAILAR, "rb")
fmailAreasConfig.Read(fmar)
fmar.close()

jsonStr = json.dumps(fmailAreasConfig.Object(), indent = 2, separators=(',', ': '), sort_keys=True)
print jsonStr

fmacObject = json.loads(jsonStr)

fmailAreasConfig2 = FmailAreasConfig()
fmailAreasConfig2.Load(fmacObject)

fmarw = open(FN_FMAILART, "wb")
fmailAreasConfig2.Write(fmarw)
fmarw.close()
