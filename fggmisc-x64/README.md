Description
====================

This plugin provides the following productivity 
key shortcuts in the FAR Editor: 

* Alt+F2   Move block left
* Alt+F3   Move block right

* Alt+F6   Search selected string in google

* Ctrl+F1  Select line and open in IE
* Ctrl+F2  Select line and open in Firefox
* Ctrl+F11 Select line and open in Chrome

* Ctrl+F3  Select word
* Ctrl+F4  Select word after the current one (can be separated by ":")

Support for Collective Knowledge Framework: http://github.com/ctuning/ck
* Ctrl+0   Select directory and open new FAR Manager in this directory
* Ctrl+1   CK: Select CID, find entry and open new FAR Manager in this entry directory 
* Ctrl+2   CK: Select CID and open Firefox with JSON meta-description 
               of this entry (CK web server should run)
* Ctrl+3   CK: Select CID and open Firefox with c-mind.org CID entry 
               (should be changed with cknowledge.org)

Developers
==========

Author and developer: Grigori Fursin

(C)opyright 2006-2015

License
=======
* This plugin for FAR Manager is distributed 
under new 3-clause BSD license.

Where to get
============
* https://github.com/ctuning/ck
* git clone https://github.com/ctuning/ck.git ck

Minimal requirements
====================
* Far > 1.70

Note, that this version was tested only on Far v3.x

Compilation
===========

* Select the build script:
** build-x86.bat for 32 bit Windows and FAR 
** build-x64.bat for 64 bit Windows and FAR

* Update build script with the path to the installed
  Microsoft Visual Studio C compiler

* Run build script

* Normally, you should see fgg.dll in either fggmisc-x64 
  or fggmisc-x86 directory

Installation
============
* Exit all FAR Manager terminals

* Open Explorer in this path

* If you use 32 bit Windows and FAR Manager, 
  copy directory fggmisc-x86
* If you use 64 bit Windows and FAR Manager, 
  copy directory fggmisc-x64

* Open Explorer in the FAR Manager root directory,
  most likely in "C:\Program Files (x86)\Far3"

* Paste directory with plugin

* Restart FAR

* Now you can use this plugin extensions when editing files
  with internal FAR Editor
