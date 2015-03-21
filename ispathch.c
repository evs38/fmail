//---------------------------------------------------------------------------
//
//  Copyright (C) 2015 Wilfred van Velzen
//
//
//  This file is part of FMail.
//
//  FMail is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  FMail is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//---------------------------------------------------------------------------

#define IS_PATH 1

unsigned char _pathtype[256] =
{
             // Dec   Hex   Char
             //
    0        // 0     00    NUL '\0'
  , 0        // 1     01    SOH (start of heading)
  , 0        // 2     02    STX (start of text)
  , 0        // 3     03    ETX (end of text)
  , 0        // 4     04    EOT (end of transmission)
  , 0        // 5     05    ENQ (enquiry)
  , 0        // 6     06    ACK (acknowledge)
  , 0        // 7     07    BEL '\a' (bell)
  , 0        // 8     08    BS  '\b' (backspace)
  , 0        // 9     09    HT  '\t' (horizontal tab)
  , 0        // 10    0A    LF  '\n' (new line)
  , 0        // 11    0B    VT  '\v' (vertical tab)
  , 0        // 12    0C    FF  '\f' (form feed)
  , 0        // 13    0D    CR  '\r' (carriage ret)
  , 0        // 14    0E    SO  (shift out)
  , 0        // 15    0F    SI  (shift in)
  , 0        // 16    10    DLE (data link escape)
  , 0        // 17    11    DC1 (device control 1)
  , 0        // 18    12    DC2 (device control 2)
  , 0        // 19    13    DC3 (device control 3)
  , 0        // 20    14    DC4 (device control 4)
  , 0        // 21    15    NAK (negative ack.)
  , 0        // 22    16    SYN (synchronous idle)
  , 0        // 23    17    ETB (end of trans. blk)
  , 0        // 24    18    CAN (cancel)
  , 0        // 25    19    EM  (end of medium)
  , 0        // 26    1A    SUB (substitute)
  , 0        // 27    1B    ESC (escape)
  , 0        // 28    1C    FS  (file separator)
  , 0        // 29    1D    GS  (group separator)
  , 0        // 30    1E    RS  (record separator)
  , 0        // 31    1F    US  (unit separator)
  , 0        // 32    20    SPACE
  , IS_PATH  // 33    21    !
  , 0        // 34    22    "
  , IS_PATH  // 35    23    #
  , 0        // 36    24    $
  , 0        // 37    25    %
  , 0        // 38    26    &
  , 0        // 39    27    ´
  , 0        // 40    28    (
  , 0        // 41    29    )
  , 0        // 42    2A    *
  , IS_PATH  // 43    2B    +
  , 0        // 44    2C    ,
  , IS_PATH  // 45    2D    -
  , IS_PATH  // 46    2E    .
  , 0        // 47    2F    /
  , IS_PATH  // 48    30    0
  , IS_PATH  // 49    31    1
  , IS_PATH  // 50    32    2
  , IS_PATH  // 51    33    3
  , IS_PATH  // 52    34    4
  , IS_PATH  // 53    35    5
  , IS_PATH  // 54    36    6
  , IS_PATH  // 55    37    7
  , IS_PATH  // 56    38    8
  , IS_PATH  // 57    39    9
  , 0        // 58    3A    :
  , 0        // 59    3B    ;
  , 0        // 60    3C    <
  , 0        // 61    3D    =
  , 0        // 62    3E    >
  , 0        // 63    3F    ?
  , IS_PATH  // 64    40    @
  , IS_PATH  // 65    41    A
  , IS_PATH  // 66    42    B
  , IS_PATH  // 67    43    C
  , IS_PATH  // 68    44    D
  , IS_PATH  // 69    45    E
  , IS_PATH  // 70    46    F
  , IS_PATH  // 71    47    G
  , IS_PATH  // 72    48    H
  , IS_PATH  // 73    49    I
  , IS_PATH  // 74    4A    J
  , IS_PATH  // 75    4B    K
  , IS_PATH  // 76    4C    L
  , IS_PATH  // 77    4D    M
  , IS_PATH  // 78    4E    N
  , IS_PATH  // 79    4F    O
  , IS_PATH  // 80    50    P
  , IS_PATH  // 81    51    Q
  , IS_PATH  // 82    52    R
  , IS_PATH  // 83    53    S
  , IS_PATH  // 84    54    T
  , IS_PATH  // 85    55    U
  , IS_PATH  // 86    56    V
  , IS_PATH  // 87    57    W
  , IS_PATH  // 88    58    X
  , IS_PATH  // 89    59    Y
  , IS_PATH  // 90    5A    Z
  , 0        // 91    5B    [
  , 0        // 92    5C    \  '\\'
  , 0        // 93    5D    ]
  , IS_PATH  // 94    5E    ^
  , IS_PATH  // 95    5F    _
  , 0        // 96    60    `
  , IS_PATH  // 97    61    a
  , IS_PATH  // 98    62    b
  , IS_PATH  // 99    63    c
  , IS_PATH  // 100   64    d
  , IS_PATH  // 101   65    e
  , IS_PATH  // 102   66    f
  , IS_PATH  // 103   67    g
  , IS_PATH  // 104   68    h
  , IS_PATH  // 105   69    i
  , IS_PATH  // 106   6A    j
  , IS_PATH  // 107   6B    k
  , IS_PATH  // 108   6C    l
  , IS_PATH  // 109   6D    m
  , IS_PATH  // 110   6E    n
  , IS_PATH  // 111   6F    o
  , IS_PATH  // 112   70    p
  , IS_PATH  // 113   71    q
  , IS_PATH  // 114   72    r
  , IS_PATH  // 115   73    s
  , IS_PATH  // 116   74    t
  , IS_PATH  // 117   75    u
  , IS_PATH  // 118   76    v
  , IS_PATH  // 119   77    w
  , IS_PATH  // 120   78    x
  , IS_PATH  // 121   79    y
  , IS_PATH  // 122   7A    z
  , 0        // 123   7B    {
  , 0        // 124   7C    |
  , 0        // 125   7D    }
  , IS_PATH  // 126   7E    ~
  , 0        // 127   7F    DEL
  , 0        // 128
  , 0        // 129
  , 0        // 130
  , 0        // 131
  , 0        // 132
  , 0        // 133
  , 0        // 134
  , 0        // 135
  , 0        // 136
  , 0        // 137
  , 0        // 138
  , 0        // 139
  , 0        // 140
  , 0        // 141
  , 0        // 142
  , 0        // 143
  , 0        // 144
  , 0        // 145
  , 0        // 146
  , 0        // 147
  , 0        // 148
  , 0        // 149
  , 0        // 150
  , 0        // 151
  , 0        // 152
  , 0        // 153
  , 0        // 154
  , 0        // 155
  , 0        // 156
  , 0        // 157
  , 0        // 158
  , 0        // 159
  , 0        // 160
  , 0        // 161
  , 0        // 162
  , 0        // 163
  , 0        // 164
  , 0        // 165
  , 0        // 166
  , 0        // 167
  , 0        // 168
  , 0        // 169
  , 0        // 170
  , 0        // 171
  , 0        // 172
  , 0        // 173
  , 0        // 174
  , 0        // 175
  , 0        // 176
  , 0        // 177
  , 0        // 178
  , 0        // 179
  , 0        // 180
  , 0        // 181
  , 0        // 182
  , 0        // 183
  , 0        // 184
  , 0        // 185
  , 0        // 186
  , 0        // 187
  , 0        // 188
  , 0        // 189
  , 0        // 190
  , 0        // 191
  , 0        // 192
  , 0        // 193
  , 0        // 194
  , 0        // 195
  , 0        // 196
  , 0        // 197
  , 0        // 198
  , 0        // 199
  , 0        // 200
  , 0        // 201
  , 0        // 202
  , 0        // 203
  , 0        // 204
  , 0        // 205
  , 0        // 206
  , 0        // 207
  , 0        // 208
  , 0        // 209
  , 0        // 210
  , 0        // 211
  , 0        // 212
  , 0        // 213
  , 0        // 214
  , 0        // 215
  , 0        // 216
  , 0        // 217
  , 0        // 218
  , 0        // 219
  , 0        // 220
  , 0        // 221
  , 0        // 222
  , 0        // 223
  , 0        // 224
  , 0        // 225
  , 0        // 226
  , 0        // 227
  , 0        // 228
  , 0        // 229
  , 0        // 230
  , 0        // 231
  , 0        // 232
  , 0        // 233
  , 0        // 234
  , 0        // 235
  , 0        // 236
  , 0        // 237
  , 0        // 238
  , 0        // 239
  , 0        // 240
  , 0        // 241
  , 0        // 242
  , 0        // 243
  , 0        // 244
  , 0        // 245
  , 0        // 246
  , 0        // 247
  , 0        // 248
  , 0        // 249
  , 0        // 250
  , 0        // 251
  , 0        // 252
  , 0        // 253
  , 0        // 254
  , 0        // 255
};
