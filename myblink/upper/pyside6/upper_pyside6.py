import sys
from PySide6.QtCore import QObject, Signal, Slot
from PySide6.QtWidgets import (
    QApplication, QTextEdit, QLabel, QPushButton, QWidget,
    QMainWindow, QFileDialog, QProgressBar, QVBoxLayout,
    QSizePolicy, QStyle
)
from PySide6.QtGui import QFont

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
    
    def SetupMessageArea(self):
        messageText = QTextEdit()
        messageText.setCursorWidth(0)
        messageText.setStyleSheet("""
            border: none
        """)
        messageText.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.layout.addWidget(messageText)


    def SetupButtons(self):
        uploadBtn = QPushButton("上传文件")
        uploadBtn.setObjectName("mybutton")
        uploadBtn.setStyleSheet(MainWindown.MYBUTTON_CLASS_STYLE)
        self.layout.addWidget(uploadBtn)
        

    def SetupUI(self):
        centralWiget = QWidget()
        self.layout = QVBoxLayout(centralWiget)
        self.SetupMessageArea()
        self.SetupButtons()
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
        pass

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindown()
    window.show()
    sys.exit(app.exec())
    print(f"window value:{window.value}")