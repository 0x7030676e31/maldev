from setuptools import setup, Extension

packageName = "maldev"

ext_module = Extension(name=packageName, sources=["lib.c"])
setup(name=packageName, version="0.1", ext_modules=[ext_module])
