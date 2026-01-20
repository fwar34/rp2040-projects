# import json, os

# class Configs():
#     def __init__(self):
#         self._blockSize = 0
#         self._uartPort = ""

#     def Load(self, configFileName):
#         print(f"curCwd:{os.getcwd()}")
#         with open(configFileName, "r") as f:
#             content = f.read()
#             configs = json.loads(content)
#             if "blockSize" in configs:
#                 self._blockSize = configs["blockSize"]
#                 print(f"blockSize:{configs["blockSize"]}")
#             if "uartPort" in configs:
#                 self._uartPort = configs["uartPort"]
#                 print(f"uartPort:{configs["uartPort"]}")

#     @property
#     def blockSize(self):
#         return self._blockSize

#     @property
#     def uartPort(self):
#         return self._uartPort

import json

class Configs:
    _instance = None
    
    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._blockSize = 0
            cls._instance._uartPort = ""
        return cls._instance

    def Load(self, configFileName):
        try:
            with open(configFileName, "r") as f:
                content = f.read()
                configs = json.loads(content)
                if "blockSize" in configs:
                    self._blockSize = configs["blockSize"]
                    print(f"blockSize:{self._blockSize}")

                if "uartPort" in configs:
                    self._uartPort = configs["uartPort"]
                    print(f"uartPort:{self._uartPort}")
        except FileNotFoundError:
            print(f"Config file '{configFileName}' not found. Using defaults.")
        except json.JSONDecodeError as e:
            print(f"Error parsing JSON from '{configFileName}': {e}")

    @property
    def blockSize(self):
        return self._blockSize

    @property
    def uartPort(self):
        return self._uartPort