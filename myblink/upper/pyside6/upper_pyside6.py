import sys
import datetime
import serial
from PySide6.QtCore import QObject, Signal, Slot, Qt, QTimer
# from PySide6.QtGui import QFont
from PySide6.QtWidgets import (
    QApplication, QTextEdit, QLabel, QPushButton, QWidget,
    QMainWindow, QFileDialog, QProgressBar, QVBoxLayout,
    QSizePolicy, QStyle, QHBoxLayout
)
import commands

class MainWindown(QMainWindow):
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
    progressValue = Signal(int)
    def __init__(self):
        super().__init__()
        self._value = 0
        self.SetupUI()
        self.setWindowTitle("测试")
        self.progressValue.connect(self.SetProgressValue)
    
    def SetupMessageArea(self):
        self.messageText = QTextEdit()
        self.messageText.setCursorWidth(0)
        self.messageText.setStyleSheet("""
            border: none;
            padding: 1px 1px;
            font-size: 13pt;
        """)
        self.messageText.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.innerVLayoutLeft.addWidget(self.messageText)

    def SetupButtons(self):
        uploadBtn = QPushButton("上传文件")
        uploadBtn.setObjectName("mybutton")
        uploadBtn.setStyleSheet(MainWindown.MYBUTTON_CLASS_STYLE)
        uploadBtn.clicked.connect(self.OnUploadBtnClicked)
        self.innerVLayoutRight.addWidget(uploadBtn)
        testBtn = QPushButton("测试进度条")
        testBtn.setObjectName("mybutton")
        testBtn.setStyleSheet(MainWindown.MYBUTTON_CLASS_STYLE)
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
        self.SetupMessageArea()
        self.SetupButtons()
        self.SetupProgress()
        self.setCentralWidget(centralWiget)

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

    @Slot()
    def OnUploadBtnClicked(self):
        fileName, _ = QFileDialog.getOpenFileName(self, "选择上传文件", "", "所有文件 (*);;文本文件 (*.txt)")
        if fileName:
            curTime = datetime.datetime.now().time().strftime("%H:%M:%S")
            self.messageText.append(f"{curTime} - {fileName}")
        else:
            self.messageText.append("未选择文件")

    @Slot()
    def OnTestBtnClicked(self):
        for i in range(1, 101):
            QTimer.singleShot(i * 20, lambda value=i: self.progressValue.emit(value))
            


if __name__ == "__main__":
    app = QApplication(sys.argv)
    screen = app.primaryScreen()
    screenWidth = screen.geometry().width()
    screenHeight = screen.geometry().height()
    appWidth = 800
    appHeight = 600
    window = MainWindown()
    window.setGeometry((screenWidth - appWidth) / 2, (screenHeight - appHeight) / 2, appWidth, appHeight)
    window.show()
    sys.exit(app.exec())
    print(f"window value:{window.value}")