#!/usr/bin/python

import struct
import time
import yaml

from datetime import datetime


#------------------------------------------------------------------------------
# Constants
#
DATATYPE_CF = 0x0102  # not used yet
DATATYPE_NO = 0x0202  # node file
DATATYPE_AD = 0x0401  # area file for echo mail defaults
DATATYPE_AE = 0x0402  # area file for echo mail

DATETIMEFMT = "%Y-%m-%d %H:%M:%S"

FN_FMAILAR      = "fmail.ar"
FN_FMAILART     = "fmail.art"
HEADERSTR       = 'FMailHeader'
FMAILAREAVERSTR = "FMail Area File rev. 1.1\x1a"

#------------------------------------------------------------------------------
def TimeStr(t):
  return time.strftime(DATETIMEFMT, time.localtime(t)) if t != 0 else "0"

#------------------------------------------------------------------------------
def Str2Time(str):
  return int(time.mktime(time.strptime(str, DATETIMEFMT)))

#------------------------------------------------------------------------------
def DissectNodeStr(nodeStr):
  (zone, sep, nodeStr) = nodeStr.rpartition(":")
  (net , sep, nodeStr) = nodeStr.rpartition("/")
  (node, sep, pnt    ) = nodeStr. partition(".")

  def ToInt(obj):
    try:
      return int(obj)
    except:
      return 0

  return ToInt(zone), ToInt(net), ToInt(node), ToInt(pnt)

#------------------------------------------------------------------------------
class NodeNum:
  nodeNumStruct = struct.Struct("<4H")

  def Write(self, f):
    f.write(self.nodeNumStruct.pack(self.zone, self.net, self.node, self.point))

  def UnPack(self, packedStr):
    (self.zone, self.net, self.node, self.point) = self.nodeNumStruct.unpack(packedStr)

  def NodeStr(self):
    if self.point > 0:
      return "%d:%d/%d.%d" % (self.zone, self.net, self.node, self.point)
    return "%d:%d/%d" % (self.zone, self.net, self.node)

  def Str2Node(self, nodeStr):
    (self.zone, self.net, self.node, self.point) = DissectNodeStr(nodeStr)

  def IsSet(self):
    return self.zone > 0 or self.net > 0 or self.node > 0

  def Print(self):
    if self.IsSet():
      print str()

#------------------------------------------------------------------------------
# typedef struct
# {
#   unsigned readOnly   : 1;  /* Bit  0    */
#   unsigned writeOnly  : 1;  /* Bit  1    */
#   unsigned locked     : 1;  /* Bit  2    */
#   unsigned reserved   : 13; /* Bits 3-15 */
# } nodeFlagsType;
#
class NodeNumX(NodeNum):
  nodeNumXStruct = struct.Struct("<8sH")

  def Write(self, f):
    NodeNum.Write(self, f)
    f.write(struct.pack("<H", self.flags))

  def UnPack(self, packedStr):
    (nodeNumStr, self.flags) = self.nodeNumXStruct.unpack(packedStr)
    NodeNum.UnPack(self, nodeNumStr)

  def Object(self):
    object = {}
    object['nodeNumber'] = self.NodeStr()
    object['flags'     ] = self.flags
    return object

  def Load(self, object):
    self.Str2Node(object['nodeNumber'])
    self.flags  = object['flags'     ]

  def Print(self):
    NodeNum.Print(self)

  def PrintX(self):
    self.Print()
    print "0x%04X" % self.flags,

#------------------------------------------------------------------------------
class RawEcho:
  s1 = struct.Struct("<HH51s51sHHB61sH59sHLHHHHH4s4sH4s4sH4s4sH4sBHBBBH6sB13sHHBBBLLLH180x")
  s2 = struct.Struct("<4B")
  s3 = struct.Struct("<3H")

  def __init__(self):
    self.forwards = []

  def Read(self, f, size):
    rawEchoStr = f.read(size)
    ok = size == len(rawEchoStr)
    if ok:
      self.UnPack(rawEchoStr)
    return ok

  def Write(self, f):
    f.write(self.s1.pack( self.signature
                        , self.writeLevel
                        , str(self.areaName)
                        , str(self.comment)
                        , self.options
                        , self.boardNumRA
                        , self.msgBaseType
                        , str(self.msgBasePath)
                        , self.board
                        , str(self.originLine)
                        , self.address
                        , self.group
                        , self.alsoSeenBy
                        , self.msgs
                        , self.days
                        , self.daysRcvd
                        , self.readSecRA
                        , self.s2.pack(*self.flagsRdRA        )
                        , self.s2.pack(*self.flagsRdNotRA     )
                        , self.writeSecRA
                        , self.s2.pack(*self.flagsWrRA        )
                        , self.s2.pack(*self.flagsWrNotRA     )
                        , self.sysopSecRA
                        , self.s2.pack(*self.flagsSysRA       )
                        , self.s2.pack(*self.flagsSysNotRA    )
                        , self.templateSecQBBS
                        , self.s2.pack(*self.flagsTemplateQBBS)
                        , self.internalUse
                        , self.netReplyBoardRA
                        , self.boardTypeRA
                        , self.attrRA
                        , self.attr2RA
                        , self.groupRA
                        , self.s3.pack(*self.altGroupRA       )
                        , self.msgKindsRA
                        , str(self.qwkName)
                        , self.minAgeSBBS
                        , self.attrSBBS
                        , self.replyStatSBBS
                        , self.groupsQBBS
                        , self.aliasesQBBS
                        , self.lastMsgTossDat
                        , self.lastMsgScanDat
                        , self.alsoSeenBy
                        , self.stat
                        ))
    for fw in self.forwards:
      fw.Write(f)

  def UnPack(self, rawEchoStr):
    ( self.signature
    , self.writeLevel
    , self.areaName
    , self.comment
    , self.options
    , self.boardNumRA
    , self.msgBaseType
    , self.msgBasePath
    , self.board
    , self.originLine
    , self.address
    , self.group
    , self.alsoSeenBy
    , self.msgs
    , self.days
    , self.daysRcvd
    , self.readSecRA
    , self.flagsRdRA         # [4]
    , self.flagsRdNotRA      # [4]
    , self.writeSecRA
    , self.flagsWrRA         # [4]
    , self.flagsWrNotRA      # [4]
    , self.sysopSecRA
    , self.flagsSysRA        # [4]
    , self.flagsSysNotRA     # [4]
    , self.templateSecQBBS
    , self.flagsTemplateQBBS # [4]
    , self.internalUse
    , self.netReplyBoardRA
    , self.boardTypeRA
    , self.attrRA
    , self.attr2RA
    , self.groupRA
    , self.altGroupRA        # [3]
    , self.msgKindsRA
    , self.qwkName
    , self.minAgeSBBS
    , self.attrSBBS
    , self.replyStatSBBS
    , self.groupsQBBS
    , self.aliasesQBBS
    , self.lastMsgTossDat
    , self.lastMsgScanDat
    , self.alsoSeenBy
    , self.stat
    ) = self.s1.unpack_from(rawEchoStr)

    # Fix strings on \0 boundaries!
    self.areaName    = self.areaName   .partition('\0')[0]
    self.comment     = self.comment    .partition('\0')[0]
    self.msgBasePath = self.msgBasePath.partition('\0')[0]
    self.originLine  = self.originLine .partition('\0')[0]
    self.qwkName     = self.qwkName    .partition('\0')[0]
    # Unpack flags
    self.flagsRdRA         = self.s2.unpack(self.flagsRdRA        )
    self.flagsRdNotRA      = self.s2.unpack(self.flagsRdNotRA     )
    self.flagsWrRA         = self.s2.unpack(self.flagsWrRA        )
    self.flagsWrNotRA      = self.s2.unpack(self.flagsWrNotRA     )
    self.flagsSysRA        = self.s2.unpack(self.flagsSysRA       )
    self.flagsSysNotRA     = self.s2.unpack(self.flagsSysNotRA    )
    self.flagsTemplateQBBS = self.s2.unpack(self.flagsTemplateQBBS)
    # Unpack altGroupRA
    self.altGroupRA        = self.s3.unpack(self.altGroupRA       )
    self.UnPackForwards(rawEchoStr[self.s1.size:])

  def UnPackForwards(self, packedStr):
    self.forwards = []
    for offset in range(0, len(packedStr), NodeNumX.nodeNumXStruct.size):
      nnx = NodeNumX()
      nnx.UnPack(packedStr[offset:offset + nnx.nodeNumXStruct.size])
      self.forwards.append(nnx)

  def Object(self):
    object = {}
    object['signature'        ] = self.signature
    object['writeLevel'       ] = self.writeLevel
    object['AreaName'         ] = self.areaName
    object['Comment'          ] = self.comment
    object['options'          ] = self.options
    object['boardNumRA'       ] = self.boardNumRA
    object['msgBaseType'      ] = self.msgBaseType
    object['MsgBasePath'      ] = self.msgBasePath
    object['board'            ] = self.board
    object['OriginLine'       ] = self.originLine
    object['address'          ] = self.address
    object['group'            ] = self.group
    object['alsoSeenBy'       ] = self.alsoSeenBy
    object['msgs'             ] = self.msgs
    object['days'             ] = self.days
    object['daysRcvd'         ] = self.daysRcvd
    object['readSecRA'        ] = self.readSecRA
    object['flagsRdRA'        ] = self.flagsRdRA
    object['flagsRdNotRA'     ] = self.flagsRdNotRA
    object['writeSecRA'       ] = self.writeSecRA
    object['flagsWrRA'        ] = self.flagsWrRA
    object['flagsWrNotRA'     ] = self.flagsWrNotRA
    object['sysopSecRA'       ] = self.sysopSecRA
    object['flagsSysRA'       ] = self.flagsSysRA
    object['flagsSysNotRA'    ] = self.flagsSysNotRA
    object['templateSecQBBS'  ] = self.templateSecQBBS
    object['flagsTemplateQBBS'] = self.flagsTemplateQBBS
    object['internalUse'      ] = self.internalUse
    object['netReplyBoardRA'  ] = self.netReplyBoardRA
    object['boardTypeRA'      ] = self.boardTypeRA
    object['attrRA'           ] = self.attrRA
    object['attr2RA'          ] = self.attr2RA
    object['groupRA'          ] = self.groupRA
    object['altGroupRA'       ] = self.altGroupRA
    object['msgKindsRA'       ] = self.msgKindsRA
    object['qwkName'          ] = self.qwkName
    object['minAgeSBBS'       ] = self.minAgeSBBS
    object['attrSBBS'         ] = self.attrSBBS
    object['replyStatSBBS'    ] = self.replyStatSBBS
    object['groupsQBBS'       ] = self.groupsQBBS
    object['aliasesQBBS'      ] = self.aliasesQBBS
    object['lastMsgTossDat'   ] = datetime.fromtimestamp(self.lastMsgTossDat)
    object['lastMsgScanDat'   ] = datetime.fromtimestamp(self.lastMsgScanDat)
    object['alsoSeenBy'       ] = self.alsoSeenBy
    object['stat'             ] = self.stat
    forwards = []
    for fw in self.forwards:
      if fw.IsSet():
        forwards.append(fw.Object())
    object['export'] = forwards
    return object

  def Load(self, object):
    self.signature         =          object['signature'        ]
    self.writeLevel        =          object['writeLevel'       ]
    self.areaName          =          object['AreaName'         ]
    self.comment           =          object['Comment'          ]
    self.options           =          object['options'          ]
    self.boardNumRA        =          object['boardNumRA'       ]
    self.msgBaseType       =          object['msgBaseType'      ]
    self.msgBasePath       =          object['MsgBasePath'      ]
    self.board             =          object['board'            ]
    self.originLine        =          object['OriginLine'       ]
    self.address           =          object['address'          ]
    self.group             =          object['group'            ]
    self.alsoSeenBy        =          object['alsoSeenBy'       ]
    self.msgs              =          object['msgs'             ]
    self.days              =          object['days'             ]
    self.daysRcvd          =          object['daysRcvd'         ]
    self.readSecRA         =          object['readSecRA'        ]
    self.flagsRdRA         =          object['flagsRdRA'        ]
    self.flagsRdNotRA      =          object['flagsRdNotRA'     ]
    self.writeSecRA        =          object['writeSecRA'       ]
    self.flagsWrRA         =          object['flagsWrRA'        ]
    self.flagsWrNotRA      =          object['flagsWrNotRA'     ]
    self.sysopSecRA        =          object['sysopSecRA'       ]
    self.flagsSysRA        =          object['flagsSysRA'       ]
    self.flagsSysNotRA     =          object['flagsSysNotRA'    ]
    self.templateSecQBBS   =          object['templateSecQBBS'  ]
    self.flagsTemplateQBBS =          object['flagsTemplateQBBS']
    self.internalUse       =          object['internalUse'      ]
    self.netReplyBoardRA   =          object['netReplyBoardRA'  ]
    self.boardTypeRA       =          object['boardTypeRA'      ]
    self.attrRA            =          object['attrRA'           ]
    self.attr2RA           =          object['attr2RA'          ]
    self.groupRA           =          object['groupRA'          ]
    self.altGroupRA        =          object['altGroupRA'       ]
    self.msgKindsRA        =          object['msgKindsRA'       ]
    self.qwkName           =          object['qwkName'          ]
    self.minAgeSBBS        =          object['minAgeSBBS'       ]
    self.attrSBBS          =          object['attrSBBS'         ]
    self.replyStatSBBS     =          object['replyStatSBBS'    ]
    self.groupsQBBS        =          object['groupsQBBS'       ]
    self.aliasesQBBS       =          object['aliasesQBBS'      ]
    self.lastMsgTossDat    = Str2Time(object['lastMsgTossDat'   ])
    self.lastMsgScanDat    = Str2Time(object['lastMsgScanDat'   ])
    self.alsoSeenBy        =          object['alsoSeenBy'       ]
    self.stat              =          object['stat'             ]
    for fw in object['export']:
      nnx = NodeNumX()
      nnx.Load(fw)
      self.forwards.append(nnx)

  def Print(self):
    print "%-51s %-61s %-59s" % (self.areaName, self.msgBasePath, self.originLine)
    for f in self.forwards:
      f.Print()
    print

#------------------------------------------------------------------------------
class Header:
  headerStruct = struct.Struct("<32s3H2i2H")

  def __init__(self):
    self.version      = FMAILAREAVERSTR
    self.rev          = 0x0100
    self.type         = DATATYPE_AE
    self.headerSize   = 50
    self.creationDate = \
    self.lastModified = int(time.time())
    self.recs         = 0
    self.recSize      = 1152

  def Read(self, f):
    headerStr = f.read(50)
    ( self.version
    , self.rev
    , self.type
    , self.headerSize
    , self.creationDate
    , self.lastModified
    , self.recs
    , self.recSize
    ) = self.headerStruct.unpack(headerStr)
    self.version = self.version.partition('\0')[0]

  def Write(self, f):
    f.write(self.headerStruct.pack( str(self.version)
                                  , self.rev
                                  , self.type
                                  , self.headerSize
                                  , self.creationDate
                                  , int(time.time())
                                  , self.recs
                                  , self.recSize
                                  )
           )

  def Object(self):
    object = {}
    object['Version'     ] = self.version
    object['rev'         ] = self.rev
    object['type'        ] = self.type
    object['headerSize'  ] = self.headerSize
    object['creationDate'] = datetime.fromtimestamp(self.creationDate)
    object['lastModified'] = datetime.fromtimestamp(self.lastModified)
    object['recs'        ] = self.recs
    object['recSize'     ] = self.recSize
    return object

  def Load(self, object):
    self.version      =          object['Version'     ]
    self.rev          =          object['rev'         ]
    self.type         =          object['type'        ]
    self.headerSize   =          object['headerSize'  ]
    self.creationDate = Str2Time(object['creationDate'])
    self.lastModified = Str2Time(object['lastModified'])
    self.recs         =          object['recs'        ]
    self.recSize      =          object['recSize'     ]

  def __str__(self):
    return HEADERSTR + ":\n" \
         + "\n" \
         + "Version      : %s\n"     % self.version               \
         + "Rev          : 0x%04X\n" % self.rev                   \
         + "Type         : 0x%04X\n" % self.type                  \
         + "Header size  : %d\n"     % self.headerSize            \
         + "Creation date: %s\n"     % TimeStr(self.creationDate) \
         + "Last modified: %s\n"     % TimeStr(self.lastModified) \
         + "# Records    : %d\n"     % self.recs                  \
         + "Record size  : %d\n"     % self.recSize


#------------------------------------------------------------------------------
class FmailAreasConfig:
  def __init__(self):
    self.header = Header()
    self.areas  = []

  def Read(self, f):
    self.__init__()
    self.header.Read(f)
    while True:
      rawEcho = RawEcho()
      if rawEcho.Read(f, self.header.recSize):
        self.areas.append(rawEcho)
      else:
        break

  def Write(self, f):
    self.header.recs = len(self.areas)
    self.header.Write(f)
    for rawEcho in self.areas:
      rawEcho.Write(f)

  def Object(self):
    self.header.recs = len(self.areas)
    object = {}
    object[HEADERSTR] = self.header.Object()
    areas = []
    for ar in self.areas:
      areas.append(ar.Object())
    object['areas'] = areas
    return object

  def Load(self, object):
    if HEADERSTR in object:
      self.header.Load(object[HEADERSTR])
    else:
      print "No header in object?"
    if 'areas' in object:
      for area in object['areas']:
        rawEcho = RawEcho()
        rawEcho.Load(area)
        self.areas.append(rawEcho)
    else:
      print "No areas in object?"

  def Print(self):
    self.header.Print()
    for rawEcho in self.areas:
      rawEcho.Print()

#------------------------------------------------------------------------------
# main
fmailAreasConfig = FmailAreasConfig()

fmar = open(FN_FMAILAR, "rb")
fmailAreasConfig.Read(fmar)
fmar.close()

output = yaml.safe_dump(fmailAreasConfig.Object(), default_flow_style=False)
print output

# fmacObject = json.loads(jsonStr)

# fmailAreasConfig2 = FmailAreasConfig()
# fmailAreasConfig2.Load(fmacObject)

# fmarw = open(FN_FMAILART, "wb")
# fmailAreasConfig2.Write(fmarw)
# fmarw.close()
