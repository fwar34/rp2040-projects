import struct

BYTE_ORDER = "<"
PKG_NAME_LEN_FIELD_BYTE_SIZE = 4
FILE_LEN_FIELD_BYTE_SIZE = 4

class CommandID(enumerate):
    COMMAND_ID_INVALID = 0
    COMMAND_ID_FILE_SEND = 1

class CommandType(enumerate):
    COMMAND_TYPE_INVALID = 0
    COMMAND_TYPE_BINARY = 1
    COMMAND_TYPE_TLV = 2
    COMMAND_JSON = 3

class BaseCommand():
    packFormat = BYTE_ORDER + "IIHHH" # headerLen + version + commandType + serviceId + commandId
    def __init__(self):
        self.headerLen = 0 # 4B
        self.version = 0 # 4B
        self.commandType = CommandType.COMMAND_TYPE_INVALID # 2B
        self.serviceId = 0 # 2B
        self.commandId = CommandID.COMMAND_ID_INVALID # 2B
        self.srcPkgLen = 0 # 2B
        self.srcPkg = "" # ascii字符个数
        self.dstPkgLen = 0 # 2B
        self.dstPkg = "" # ascii字符个数

    def bytesLen(self):
        headerLen = struct.calcsize(self.packFormat)
        srcPkgLen = len(self.srcPkg)
        headerLen = headerLen + PKG_NAME_LEN_FIELD_BYTE_SIZE + srcPkgLen
        dstPkgLen = len(self.dstPkg)
        headerLen = headerLen + PKG_NAME_LEN_FIELD_BYTE_SIZE + dstPkgLen

        return headerLen

    def to_bytes(self):
        self.headerLen = struct.calcsize(self.packFormat)
        self.srcPkgLen = len(self.srcPkg)
        self.headerLen = self.headerLen + PKG_NAME_LEN_FIELD_BYTE_SIZE + self.srcPkgLen
        self.dstPkgLen = len(self.dstPkg)
        self.headerLen = self.headerLen + PKG_NAME_LEN_FIELD_BYTE_SIZE + self.dstPkgLen

        retBytes = struct.pack(self.packFormat, self.headerLen, self.version, self.commandType,
            self.serviceId, self.commandId)
        retBytes = retBytes + struct.pack(BYTE_ORDER + "H", self.srcPkgLen)
        if self.srcPkgLen != 0:
            retBytes = retBytes + struct.pack(BYTE_ORDER + f"{self.srcPkgLen}s", self.srcPkg)

        retBytes = retBytes + struct.pack(BYTE_ORDER + "H", self.dstPkgLen)
        if self.dstPkgLen != 0:
            retBytes = retBytes + struct.pack(BYTE_ORDER + f"{dstPkgLen}s", self.dstPkgLen)
        return retBytes

class FileCommand(BaseCommand):
    def __init__(self):
        super().__init__()
        self.headerLen = self.bytesLen()
        self.commandType = CommandType.COMMAND_TYPE_BINARY
        self.commandId = CommandID.COMMAND_ID_FILE_SEND
        self.fileLen = 0

    def bytesLen(self):
        return super().bytesLen() + FILE_LEN_FIELD_BYTE_SIZE

    def to_bytes(self):
        retBytes = super().to_bytes() + struct.pack(BYTE_ORDER + "I", self.fileLen)
        return retBytes