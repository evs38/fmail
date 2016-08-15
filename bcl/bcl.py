#!/usr/bin/python

#import json
import struct
import time

#------------------------------------------------------------------------------
# Constants
#
BCLH_ISLIST   = 0x00000001  # File is complete list
BCLH_ISUPDATE = 0x00000002  # File contains update/diff information

FLG1_READONLY = 0x00000001  # Read only, software must not allow users to enter mail.
FLG1_PRIVATE  = 0x00000002  # Private attribute of messages is honored.
FLG1_FILECONF = 0x00000004  # File conference.
FLG1_MAILCONF = 0x00000008  # Mail conference.
FLG1_REMOVE   = 0x00000010  # Remove specified conference from list (otherwise add/upd).

DATETIMEFMT = "%Y-%m-%d %H:%M:%S"

FINGERPRINTSTR = 'BCL\0'
MYCONFMGRNAME  = 'bcl.py'

MINLEN = 51

#------------------------------------------------------------------------------
def TimeStr(t):
  return time.strftime(DATETIMEFMT, time.gmtime(t))

#------------------------------------------------------------------------------
def Str2Time(str):
  return int(time.mktime(time.strptime(str, DATETIMEFMT)))

def MinLen(str):
  return min(len(str), MINLEN)

#------------------------------------------------------------------------------
# struct bcl_header {
#   char    FingerPrint[4];     /* BCL<NUL> */
#   char    ConfMgrName[31];    /* Name of "ConfMgr" */
#   char    Origin[51];         /* Originating network addr */
#   long    CreationTime;       /* UNIX-timestamp when created */
#   long    flags;              /* Options, see below */
#   char    Reserved[256];      /* Reserved data */
# }
#
class BCL_HEADER:
  s_header = struct.Struct("<4s31s51sll256x")

  def __init__(self):
    self.FingerPrint  = FINGERPRINTSTR
    self.ConfMgrName  = MYCONFMGRNAME
    self.Origin       = ''
    self.CreationTime = time.time()
    self.flags        = BCLH_ISLIST

  def Read(self, f):
    headerStr = f.read(self.s_header.size)
    ( self.FingerPrint
    , self.ConfMgrName
    , self.Origin
    , self.CreationTime
    , self.flags
    ) = self.s_header.unpack(headerStr)

  def Write(self, f):
    f.write(self.s_header.pack(self.FingerPrint, self.ConfMgrName, self.Origin, time.time(), self.flags))

  def Print(self):
    print "FingerPrint :", self.FingerPrint
    print "ConfMgrName :", self.ConfMgrName
    print "Origin      :", self.Origin
    print "CreationTime:", TimeStr(self.CreationTime)
    print "flags       :", self.flags

#------------------------------------------------------------------------------
# struct bcl {
#   int     EntryLength;      /* Length of entry data */
#   long    flags1, flags2;   /* Conference flags */
#   char    *AreaTag;         /* Area tag [51] */
#   char    *Description;     /* Description [51] */
#   char    *Administrator;   /* Administrator or contact [51] */
# }
#
class BCL_ENTRY:
  s_bcl = struct.Struct("<hll")

  def __init__(self, AreaTag = '', Description = ''):
    self.EntryLength   = 0
    self.flags1        = FLG1_MAILCONF
    self.flags2        = 0
    self.AreaTag       = AreaTag
    self.Description   = Description
    self.Administrator = ''

  def Read(self, f):
    bclStr = f.read(self.s_bcl.size)
    if len(bclStr) != self.s_bcl.size:
      return False

    ( self.EntryLength
    , self.flags1
    , self.flags2
    ) = self.s_bcl.unpack(bclStr)
    bclStr = f.read(self.EntryLength - self.s_bcl.size)
    if len(bclStr) != self.EntryLength - self.s_bcl.size:
      return False

    ( self.AreaTag
    , self.Description
    , self.Administrator
    ) = bclStr.split('\0')[0:3]

    return True

  def Write(self, f):
    self.EntryLength = self.s_bcl.size + 3 + MinLen(self.AreaTag) + MinLen(self.Description) + MinLen(self.Administrator)
    f.write(self.s_bcl.pack(self.EntryLength, self.flags1, self.flags2))
    f.write(self.AreaTag[:MINLEN])
    f.write('\0')
    f.write(self.Description[:MINLEN])
    f.write('\0')
    f.write(self.Administrator[:MINLEN])
    f.write('\0')

  def WriteTag(self, f, l):
    print >> f, "%-*s %s" % (l, self.AreaTag, self.Description)

  def Print(self):
#   print "EntryLength  :", self.EntryLength
    print "flags1       :", self.flags1
    print "flags2       :", self.flags2
    print "AreaTag      :", self.AreaTag
    print "Description  :", self.Description
    print "Administrator:", self.Administrator

#------------------------------------------------------------------------------
class BCL_FILE:
  def __init__(self):
    self.header = BCL_HEADER()
    self.bcls   = []

  def Read(self, f):
    self.__init__()
    self.header.Read(f)
    while True:
      bcle = BCL_ENTRY()
      if bcle.Read(f):
        self.bcls.append(bcle)
      else:
        break

  def Write(self, f):
    self.header.Write(f)
    for bcle in self.bcls:
      bcle.Write(f)

  def WriteTagFile(self, f):
    lmax = 0
    for bcle in self.bcls:
      if len(bcle.AreaTag) > lmax:
        lmax = len(bcle.AreaTag)
    for bcle in self.bcls:
      bcle.WriteTag(f, lmax)

  def Print(self):
    print "BCL_HEADER:"
    print
    self.header.Print()
    print "-------------------------------------------"
    for bcle in self.bcls:
      bcle.Print()
      print

#------------------------------------------------------------------------------
def ReadTagFile(f):
  bclf = BCL_FILE()
  for tagline in f:
    (AreaTag, Description) = tagline.split(None, 1)
    bclf.bcls.append(BCL_ENTRY(AreaTag, Description.rstrip()))

  return bclf

#------------------------------------------------------------------------------
# main
bclf = BCL_FILE()

fbcl = open("TEST.BCL", "rb")
bclf.Read(fbcl)
fbcl.close()

bclf.Print()

fbcl = open("TEST.TAG", "wb")
bclf.WriteTagFile(fbcl)
fbcl.close()

fbcl = open("TEST2.TAG", "rb")
bclf = ReadTagFile(fbcl)
fbcl.close()
bclf.header.Origin = '2:280/5003'
fbcl = open("TEST2.BCL", "wb")
bclf.Write(fbcl)
fbcl.close()

fbcl = open("TEST2.BCL", "rb")
bclf.Read(fbcl)
fbcl.close()
fbcl = open("TEST2o.TAG", "wb")
bclf.WriteTagFile(fbcl)
fbcl.close()

#------------------------------------------------------------------------------
