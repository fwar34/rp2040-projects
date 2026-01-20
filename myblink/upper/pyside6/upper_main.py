import sys, os, json, io
import datetime
from PySide6.QtCore import QObject, Signal, Slot, Qt, QTimer
from PySide6.QtWidgets import (
    QApplication, QTextEdit, QLabel, QPushButton, QWidget,
    QMainWindow, QFileDialog, QProgressBar, QVBoxLayout,
    QSizePolicy, QStyle, QHBoxLayout
)
import upload_thread
import configs

# 设置标准输出的编码为UTF-8
# sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# 构建配置文件的正确路径
# scriptDir = os.path.dirname(os.path.abspath(__file__))
# configFilePath = os.path.join(os.path.dirname(scriptDir), "upper.json")
configFilePath = "./upper.json"
conf = configs.Configs()

class MainWindow(QMainWindow):
    signalLogMessage = Signal(str)

    MYBUTTON_CLASS_STYLE = """
        #mybutton {
            background-color: #4CAF50;
            border: none;
            color: white;
            padding: 10px 20px;
            text-align: center;
            font-size: 16px;
            border-radius: 5px;
        }

        #mybutton:hover {
            background-color: orange;
        }

        #mybutton:pressed {
            background-color: red;
        }
    """
    # progressValue = Signal(int)
    def __init__(self):
        super().__init__()
        self._value = 0
        self.SetupUI()
        self.setWindowTitle("测试")
        self.uploadThread = upload_thread.UploadThread(self)
        self.uploadThread.GetProgressSignal().connect(self.SetProgressValue)
        self.signalLogMessage.connect(self.SlotDisPlayLog)
    
    def SetupFileMessageArea(self):
        # 上面的文件列表
        fileLabel = QLabel()
        fileLabel.setText("上传文件信息")
        fileLabel.setStyleSheet("""
            font-size: 13pt
        """)
        self.innerVLayoutLeft.addWidget(fileLabel)
        self.messageText = QTextEdit()
        self.messageText.setCursorWidth(0)
        self.messageText.setStyleSheet("""
            border: none;
            padding: 1px 1px;
            font-size: 13pt;
        """)
        self.messageText.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.innerVLayoutLeft.addWidget(self.messageText)
        # 下面的日志输出
        logLabel = QLabel()
        logLabel.setText("日志输出")
        logLabel.setStyleSheet("""
            font-size: 13pt
        """)
        self.innerVLayoutLeft.addWidget(logLabel)
        self.logText = QTextEdit()
        self.logText.setCursorWidth(0)
        self.logText.setStyleSheet("""
            border: none;
            padding: 1px 1px;
            font-size: 13pt;
        """)
        self.logText.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.innerVLayoutLeft.addWidget(self.logText)

    def SetupButtons(self):
        uploadBtn = QPushButton("上传文件")
        uploadBtn.setObjectName("mybutton")
        uploadBtn.setStyleSheet(MainWindow.MYBUTTON_CLASS_STYLE)
        uploadBtn.clicked.connect(self.OnUploadBtnClicked)
        self.innerVLayoutRight.addWidget(uploadBtn)
        cancelBtn = QPushButton("取消上传")
        cancelBtn.setObjectName("mybutton")
        cancelBtn.setStyleSheet(MainWindow.MYBUTTON_CLASS_STYLE)
        cancelBtn.clicked.connect(self.OnCancelBtnClicked)
        testBtn = QPushButton("测试进度条")
        testBtn.setObjectName("mybutton")
        testBtn.setStyleSheet(MainWindow.MYBUTTON_CLASS_STYLE)
        testBtn.clicked.connect(self.OnTestBtnClicked)
        self.innerVLayoutRight.addWidget(testBtn)

    
    def SetupProgress(self):
        self.progress = QProgressBar()
        self.progress.setStyleSheet("""
            QProgressBar {
                height: 24px;
                border: 1px solid #ddd;
                border-radius: 12px;  /* 圆角半径=高度/2,做胶囊状 */
                background-color: #f5f5f5;
                text-align: center;
            }
            QProgressBar::chunk {
                background-color: #2196F3;
                border-radius: 11px;  /* 比外层小1px,避免边框溢出 */
                margin: 0px;  /* 关键：去掉边距 */
            }
        """)
        self.progress.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.innerVLayoutLeft.addWidget(self.progress)

    def SetupUI(self):
        centralWiget = QWidget()
        self.outHLayout = QHBoxLayout(centralWiget)
        self.innerVLayoutLeft = QVBoxLayout()
        self.innerVLayoutLeft.setAlignment(Qt.AlignTop)
        self.innerVLayoutRight = QVBoxLayout()
        self.innerVLayoutRight.setAlignment(Qt.AlignTop)
        self.outHLayout.addLayout(self.innerVLayoutLeft)
        self.outHLayout.addLayout(self.innerVLayoutRight)
        self.SetupFileMessageArea()
        self.SetupButtons()
        self.SetupProgress()
        self.setCentralWidget(centralWiget)

    def LogDisplay(self, message):
        self.signalLogMessage.emit(message)

    @Slot(str)
    def SlotDisPlayLog(self, message):
        self.logText.append(message)

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, newValue):
        if isinstance(newValue, int) and newValue >= 0:
            self._value = newValue
        else:
            raise ValueError("Value must be a non-negative integer")

    @Slot(int)
    def SetProgressValue(self, value):
        if hasattr(self, 'progress'):
            self.progress.setValue(value)
kk
    @Slot()
    def OnUploadBtnClicked(self):
        fileName, _ = QFileDialog.getOpenFileName(self, "选择上传文件", "", "所有文件 (*);;文本文件 (*.txt)")
        if fileName:
            # curTime = datetime.datetime.now().time().strftime("%H:%M:%S")
            self.messageText.append(f"{fileName} - {os.path.getmtime(fileName)}, 大小: {os.path.getsize(fileName)}B")
            self.uploadThread.StartThread(fileName)
        else:
            self.messageText.append("未选择文件")

    @Slot()
    def OnCancelBtnClicked(self):
        self.uploadThread.StopThread()

    @Slot()
    def OnTestBtnClicked(self):
        for i in range(1, 101):
            QTimer.singleShot(i * 20, lambda value=i: self.uploadThread.GetProgressSignal().emit(value))

if __name__ == "__main__":
    print(f"curCwd:{os.getcwd()}")
    conf.Load(configFilePath)

    app = QApplication(sys.argv)
    screen = app.primaryScreen()
    screenWidth = screen.geometry().width()
    screenHeight = screen.geometry().height()
    appWidth = 800
    appHeight = 600
    window = MainWindow()
    window.setGeometry((screenWidth - appWidth) / 2, (screenHeight - appHeight) / 2, appWidth, appHeight)
    window.show()
    sys.exit(app.exec())