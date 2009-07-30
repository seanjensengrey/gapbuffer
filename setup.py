from distutils.core import setup, Extension

module1 = Extension('gapbuffer',
                    sources = ['gapbuffer.c'])

setup (name = 'gapbuffer',
       version = '1.03',
       description = 'A gap buffer package',
       long_description = """Gap buffers are efficient mutable sequences that 
       are often used to store text in text editors. 
       They utilize locality of modification to avoid copying large amounts 
       of data and allocate extra elements to avoid memory allocation 
       dominating performance.""",
       author = "Neil Hodgson",
       author_email = "neilh@scintilla.org",
       license = "MIT",
       url = "http://code.google.com/p/gapbuffer/",
       download_url = "http://gapbuffer.googlecode.com/files/gapbuffer-1.03.zip",
       ext_modules = [module1],
       platforms = ["Platform Independent"],
       classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Intended Audience :: Developers',
          'License :: OSI Approved :: MIT License',
          'Operating System :: OS Independent',
          'Programming Language :: C',
          'Programming Language :: Python',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 3',
          'Topic :: Software Development :: Libraries',
       ]
)

