from PySide6.QtCore import QThread, QMutex, QMutexLocker, QWaitCondition, Signal, Slot
import commands
import configs
import serial_send
from upper_main import MainWindow
import os

conf = configs.Configs()

class UploadThread(QThread):
    progressValue = Signal(int)
    
    def __init__(self, mainWindow, parent=None):
        super().__init__(parent=parent)
        self.running = False
        self.filePath = ""
        self.mutex = QMutex()
        self.waitCond = QWaitCondition()
        self.serialSend = serial_send.SerialSend()
        self.mainWindow = mainWindow
        super().start()

    def GetProgressSignal(self):
        return self.progressValue

    def PauseThread(self):
        with QMutexLocker(self.mutex):
            self.running = False

    def ResumeThread(self):
        with QMutexLocker(self.mutex):
            if not self.running:
                self.running = True
                self.waitCond.notify_one()

    def StartThread(self, filePath):
        with QMutexLocker(self.mutex):
            self.filePath = filePath
            self.running = True
            self.waitCond.notify_one()

    def run(self):
        while True:
            with QMutexLocker(self.mutex):
                while not self.running:
                    self.waitCond.wait(self.mutex)
                self.UploadFile(self.filePath)
                self.running = False
    
    def UploadFile(self, fileName):
        if not self.serialSend.Connect(conf.uartPort):
            return
        self.mainWindow.LogDisplay("串口连接成功")
        with open(fileName, "rb") as f:
            fileSize = os.path.getsize(fileName)
            blockCount = int(fileSize / conf.blockSize)
            if fileSize % conf.blockSize != 0:
                blockCount += 1
            sendCount = 0
            for i in range(0, blockCount):
                with QMutexLocker(self.mutex):
                    if not self.isRunning:
                        self.mainWindow.LogDisplay(f"停止上传文件:{fileName}")
                        return
                fileCommand = commands.FileCommand()
                fileCommand.fileLen = fileSize
                fileCommand.blockOffset = f.tell()
                if i < blockCount - 1:
                    fileCommand.blockLen = conf.blockSize
                else: #最后一个 block
                    fileCommand.blockLen = fileSize % conf.blockSize
                fileCommand.byteArray = f.read(fileCommand.blockLen)
                sendRet = self.serialSend.Send(fileCommand.to_bytes())
                if sendRet < serial_send.SerialError.SERIAL_ERROR_SUCCESS:
                    self.mainWindow.LogDisplay(f"串口发送失败:{sendRet}")
                    return
                sendCount += len(fileCommand.byteArray)
                self.progressValue.emit(sendCount / fileSize * 100)
                print(f"progressValue.emit:{sendCount / fileSize * 100}, sendCount:{sendCount}, fileSize:{fileSize}")
            self.mainWindow.LogDisplay(f"文件{fileName}传输完成，文件大小{fileSize}, blockCount:{blockCount}")