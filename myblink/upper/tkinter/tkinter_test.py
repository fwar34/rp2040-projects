import tkinter as tk

root = tk.Tk()
root.title("Grid 布局示例")

# 创建多个按钮演示不同参数
btn1 = tk.Button(root, text="按钮1", bg="red")
btn1.grid(row=0, column=0, padx=5, pady=5)

btn2 = tk.Button(root, text="按钮2", bg="green")
btn2.grid(row=0, column=1, padx=5, pady=5)

btn3 = tk.Button(root, text="按钮3", bg="blue")
btn3.grid(row=1, column=0, columnspan=2, sticky="ew", padx=5, pady=5)

# 文本框占据整个区域
text = tk.Text(root, height=5)
text.grid(row=2, column=0, columnspan=2, sticky="nsew", padx=5, pady=5)

# 配置权重使组件能够响应窗口大小变化
root.grid_rowconfigure(2, weight=1)  # 第2行可扩展
root.grid_columnconfigure(0, weight=1)  # 第0列可扩展
root.grid_columnconfigure(1, weight=1)  # 第1列可扩展

root.mainloop()