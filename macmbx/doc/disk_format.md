# Disk Format

Binary TOC format specification. Compatible with Eudora 6/7/8.

## File Layout

```
+----------------------------+
| TOCDiskHeader (76 bytes)   |
+----------------------------+
| MSumDisk[0]  (224 bytes)   |
| MSumDisk[1]  (224 bytes)   |
| ...                        |
| MSumDisk[N-1] (224 bytes)  |
+----------------------------+
```

Total file size: `76 + count * 224` bytes.

## TOCDiskHeader (76 bytes)

```c
typedef struct __attribute__((packed)) {
  int32_t majorVersion;     /*  0: version major (current: 1) */
  int32_t minorVersion;     /*  4: version minor (current: 9) */
  int16_t count;            /*  8: number of messages */
  int16_t which;            /* 10: mailbox type (0=normal, 1=In, 2=Out, 3=Trash, 4=Junk) */
  int32_t boxSize;          /* 12: mbox file size at time of write (+1) */
  int32_t writeDate;        /* 16: timestamp of TOC write */
  int32_t nextSerialNum;    /* 20: next serial number to assign */
  int32_t sort;             /* 24: current sort order */
  int32_t lastSort;         /* 28: previous sort order */
  int32_t pluginKey;        /* 32: plugin identifier */
  int32_t pluginValue;      /* 36: plugin data */
  int32_t previewHi;        /* 40: preview range high */
  int32_t unreadBase;       /* 44: unread base count */
  int32_t sorts[6];         /* 48: sort state array (24 bytes) */
  int32_t needsCompact;     /* 72: compaction needed flag */
};                          /* Total: 76 bytes */
```

## MSumDisk (224 bytes)

```c
typedef struct __attribute__((packed)) {
  int32_t  offset;          /*   0: byte offset in mbox file */
  int32_t  length;          /*   4: message length in bytes */
  int32_t  bodyOffset;      /*   8: body start relative to offset */
  int32_t  state;           /*  12: message state (0=unread..10=rebuilt) */
  int32_t  spamBits;        /*  16: spamScore:8 | spamBecause:3 | spare:21 */
  uint32_t arrivalSeconds;  /*  20: arrival time (UTC seconds) */
  uint32_t mesgErrH;        /*  24: always 0 (was Mac Handle) */
  uint32_t fromHash;        /*  28: hash of From address */
  uint32_t spare[3];        /*  32: reserved (12 bytes) */
  int32_t  serialNum;       /*  44: unique serial number */
  uint32_t seconds;         /*  48: Date: header as UTC seconds */
  uint32_t flags;           /*  52: flag bitfield */
  int16_t  savedPos[4];     /*  56: window position rect (8 bytes) */
  uint8_t  priority;        /*  64: priority 1-5 */
  uint8_t  origPriority;    /*  65: original priority */
  int16_t  tableId;         /*  66: character table ID */
  int16_t  scoreBits;       /*  68: score:4 | outType:4 | unused:8 */
  int16_t  spareShort2;     /*  70: reserved */
  int16_t  sumRandBytes;    /*  72: random bytes */
  int16_t  origZone;        /*  74: timezone offset in minutes */
  uint32_t sigId;           /*  76: signature ID */
  char     from[48];        /*  80: sender display string */
  uint32_t popPersId;       /* 128: receiving personality ID */
  uint32_t persId;          /* 132: sending personality ID */
  int32_t  msgIdHash;       /* 136: hash of Message-ID */
  int16_t  subjId;          /* 140: subject ID */
  int16_t  spareShort;      /* 142: reserved */
  char     subj[60];        /* 144: subject string */
  uint32_t opts;            /* 204: option flags */
  uint32_t uidHash;         /* 208: UIDL hash */
  uint32_t cache;           /* 212: always 0 (was Mac Handle) */
  uint8_t  selected;        /* 216: selection state */
  uint8_t  _pad[3];         /* 217: padding */
  uint32_t messH;           /* 220: always 0 (was Mac Handle) */
};                          /* Total: 224 bytes */
```

## Byte Order

All integers are stored in **native byte order** (little-endian on x86/ARM). This matches Eudora's behavior -- Eudora never handled cross-platform TOC files.

## Version History

| Major | Minor | Changes |
|-------|-------|---------|
| 0 | * | Pre-release, no uidHash or msgIdHash |
| 1 | 0 | Added uidHash |
| 1 | 1 | Added msgIdHash |
| 1 | 2 | Added serial numbers |
| 1 | 5 | Added spam score fields |
| 1 | 9 | Current version |

## Validation Rules

A TOC is considered valid if:
1. File size >= 76 (header present)
2. File size >= 76 + count * 224 (all summaries present)
3. `count` is 0..100000
4. `majorVersion` <= 1
5. For each summary: `offset >= 0`, `length >= 0`, `bodyOffset >= 0`, `bodyOffset <= length`, `offset + length <= mbox_file_size`
6. `boxSize` approximately matches actual mbox file size (within 1 byte)

## Mbox File Format

Standard Unix mbox (mboxo variant):
- Messages separated by "From " lines
- Format: `From sender@host Day Mon DD HH:MM:SS YYYY\n`
- Message body follows (RFC822 headers + blank line + body)
- "From " at start of body lines is NOT escaped (mboxo, not mboxrd)

## Conversion Functions

```c
void macmbx_disk_to_sum(const MacmbxDiskSum *disk, MacmbxMsgSum *mem);
void macmbx_sum_to_disk(const MacmbxMsgSum *mem, MacmbxDiskSum *disk);
```

These handle bitfield packing/unpacking (spamBits, scoreBits) and zero out pointer fields (mesgErrH, cache, messH) on write.
