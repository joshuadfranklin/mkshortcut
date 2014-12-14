
I was an avid Cygwin user for many years, and a project contributor from roughly 2001 to 2006.
I now rarely use Windows but still think Cygwin and derivatives like MobaXterm make using Windows significantly less painful.


* Wrote mkshortcut, a GNU-style Cygwin command line utility to create a Shortcut (OLE Shell Link)
  * Included in cygutils with small changes over the years
  * Handy options such as "All Users" and "Start Menu/Programs"
  * I was not great at debugging https://cygwin.com/ml/cygwin/2002-03/msg00055.html
  * Example: Create a shortuct to rxvt on the Desktop that has Internet Explorer's icon but really starts up bash:
```
mkshortcut -a '-rv -fn "FixedSys" -e /bin/bash --login -i' \
-i /c/WINNT/system32/SHELL32.DLL -j 106 -n "Internet Explorer" \
-D /bin/rxvt
```

* Maintained and added to the Cygwin documenation 
  * User Guide, FAQs, cygwin-doc package building and distribution
  * Gold Stars https://cygwin.com/goldstars/

* Created Cygwin-lite Before Cygwin's setup.exe installer did a minimal install
  * http://cygwin-lite.sourceforge.net/
  * Beginner's Guide tutorial http://cygwin-lite.sourceforge.net/html/begin.html
  * Fit on a 3.5" floppy, and on my tiny computer lab network space
