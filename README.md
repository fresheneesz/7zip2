7zip2
=====

7-Zip is a file archiver for Windows.

This repository is a fork of the [7zip SourceForge project](https://sourceforge.net/projects/sevenzip/) (http://www.7-zip.org) made because the maintainers on sourceforge have ignored large problems in their system for too long, and sourceforge is so incredibly dated.

Please look here to find out how to build the source on windows: http://www.ski-epic.com/2012_compiling_7zip_on_windows_with_visual_studio_10/index.html

CONTRIBUTORS NEEDED
===================

I don't have the bandwidth to actively move this project forward. If anyone is interested in collaborating in this project please contact me. 

Description of 7-Zip sources package
===================================

##  Documentation

  7zFormat.txt   - 7z format description
  copying.txt    - GNU LGPL license
  unRarLicense.txt - License for unRAR part of source code
  history.txt    - Sources history
  Methods.txt    - Compression method IDs
  readme.txt     - Readme file
  lzma.txt       - LZMA SDK description
  7zip.nsi       - installer script for NSIS


C   - Source code in C
CPP - Source code in CPP

Common            Common modules
Windows           Win32 wrappers

## 7zip

### Common
Common modules for 7-zip

### Archive
7-Zip Archive Format Plugins

    Common
    7z
    Arj
    BZip2
    Cab
    Cpio
    GZip
    Rar
    Rpm
    Split
    Tar
    Zip

###Bundle
Modules that are bundles of other modules

    Alone         7za.exe: Standalone version of 7z
    Alone7z       7zr.exe: Standalone version of 7z that supports only 7z/LZMA/BCJ/BCJ2
    SFXCon        7zCon.sfx: Console 7z SFX module
    SFXWin        7z.sfx: Windows 7z SFX module
    SFXSetup      7zS.sfx: Windows 7z SFX module for Installers
    Format7z            7za.dll:  .7z support
    Format7zExtract     7zxa.dll: .7z support, extracting only
    Format7zR           7zr.dll:  .7z support, LZMA/BCJ* only
    Format7zExtractR    7zxr.dll: .7z support, LZMA/BCJ* only, extracting only
    Format7zF           7z.dll:   all formats

###UI
    Agent         Intermediary modules for FAR plugin and Explorer plugin
    Console       7z.exe Console version
    Explorer      Explorer plugin
    Resource      Resources
    Far           FAR plugin
    Client7z      Test application for 7za.dll

###Compress
    BZip2        BZip2 compressor
    Branch       Branch converter
    ByteSwap     Byte Swap converter
    Copy         Copy coder
    Deflate
    Implode
    Arj
    LZMA
    PPMd          Dmitry Shkarin's PPMdH with small changes.
    LZ            Lempel - Ziv

###Crypto
Crypto modules

    7zAES         Cipher for 7z
    AES           AES Cipher
    Rar20         Cipher for Rar 2.0
    RarAES        Cipher for Rar 3.0
    Zip           Cipher for Zip

  FileManager       File Manager

Contribute
===========

I know almost nothing about windows GUI programming, and so I'm very out of my element here. I'm looking for people to be at least partial owners of this project, especially if the original creators want to move their work to github (which I imagine they won't).

Feel free to write issues, submit pull requests, and update this documentation.


Todo
=====

See the issues for other things to do.

License
========
7-Zip is free software distributed under the GNU LGPL (except for unRar code). See the LICENSE file for more info.

Notes about unRAR license:

Please check main restriction from unRar license:

      The unRAR sources may be used in any software to handle RAR
      archives without limitations free of charge, but cannot be used
      to re-create the RAR compression algorithm, which is proprietary.
      Distribution of modified unRAR sources in separate form or as a
      part of other software is permitted, provided that it is clearly
      stated in the documentation and source comments that the code may
      not be used to develop a RAR (WinRAR) compatible archiver.

In brief it means:
1) You can compile and use compiled files under GNU LGPL rules, since 
   unRAR license almost has no restrictions for compiled files.
   You can link these compiled files to LGPL programs.
2) You can fix bugs in source code and use compiled fixed version.
3) You can not use unRAR sources to re-create the RAR compression algorithm.

LZMA SDK
--------

Also this package contains files from LZMA SDK
you can download LZMA SDK from this page:
http://www.7-zip.org/sdk.html
read about addtional licenses for LZMA SDK in file
DOC/lzma.txt

Notes
------
7-Zip consists of COM modules (DLL files).
But 7-Zip doesn't use standard COM interfaces for creating objects.
Look at
7zip\UI\Client7z folder for example of using DLL files of 7-Zip. 
Some DLL files can use other DLL files from 7-Zip.
If you don't like it, you must use standalone version of DLL.
To compile standalone version of DLL you must include all used parts
to project and define some defs. 
For example, 7zip\Bundles\Format7z is a standalone version  of 7z.dll 
that works with 7z format. So you can use such DLL in your project 
without additional DLL files.

Copyright
==========

Even tho 7-zip is released under the GPL, Igor has a copyright as follows:

7-Zip Copyright (C) 1999-2010 Igor Pavlov.
