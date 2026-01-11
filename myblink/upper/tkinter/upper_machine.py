import serial
import tkinter
from tkinter import filedialog
import commands
import os

root = tkinter.Tk()
root.geometry("800x600")
root.title("测试")

messageBox = tkinter.Text(root)
messageBox.grid(row=0, column=0, sticky="new", padx=5, pady=5)

def Message(messageBox: tkinter.Text, message: str, newline=True, erase=True):
    if erase:
        messageBox.delete("1.0", tkinter.END)
        messageBox.insert("1.0", message)
    else:
        if newline:
            message = "\n" + message if newline else message
        messageBox.insert(tkinter.END, message)

def on_click():
    file_path = filedialog.askopenfilename(title="选择文件上传", filetypes=[("All files", "*.*")])
    if file_path:
        with open(file_path) as f:
            fileCommand = commands.FileCommand()
            fileCommand.fileLen = os.path.getsize(file_path)
            message = f"选择的文件:{file_path}, 大小:{fileCommand.fileLen}"
            Message(messageBox, message, newline=True, erase=False)
            print(message)

    else:
        print("未选择文件")


btnFileChoose = tkinter.Button(root, text="上传文件", command=on_click)
btnFileChoose.grid(row=0, column=1, padx=5)
root.mainloop()
