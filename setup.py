from distutils.core import setup, Extension

module1 = Extension('gapbuffer',
                    sources = ['gapbuffer.c'])

setup (name = 'gapbuffer',
       version = '1.01',
       description = 'A gap buffer package',
       author = "Neil Hodgson",
       author_email = "neilh@scintilla.org",
       license = "MIT",
       url = "http://code.google.com/p/gapbuffer/",
       download_url = "http://gapbuffer.googlecode.com/files/gapbuffer-1.01.zip",
       ext_modules = [module1])

