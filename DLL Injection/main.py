import os

path = os.path.dirname(os.path.realpath(__file__)) + "\\sample.dll"
module = __import__("maldev")
module.inject(path)
