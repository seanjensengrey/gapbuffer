from distutils.core import setup, Extension

module1 = Extension('gapbuffer',
                    sources = ['gapbuffer.c'])

setup (name = 'gapbuffer',
       version = '1.0',
       description = 'A gap buffer package',
       author = "Neil Hodgson",
       author_email = "neilh@scintilla.org",
       license = "MIT",
       url = "http://www.scintilla.org/gapbuffer.html",
       download_url = "http://www.scintilla.org/gapbuffer-1.0.zip",
       ext_modules = [module1])

