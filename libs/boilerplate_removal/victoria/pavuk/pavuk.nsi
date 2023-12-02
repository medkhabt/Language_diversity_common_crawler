Name "Pavuk"
OutFile pavukinst.exe
ComponentText "This program will install Pavuk on your system."
SilentInstall 'normal'
CRCCheck on
UninstallText 'This will uninstall Pavuk from your system.'
UninstallExeName uninstall.exe
DirText 'Select destination directory'
LicenseData COPYING
LicenseText 'Pavuk is distributed under GPL. Read the licensing terms below'

InstallDir $PROGRAMFILES\Pavuk
InstallDirRegKey HKEY_LOCAL_MACHINE 'Software\Stefan Ondrejicka\Pavuk' 'Install Path'

Section 'Pavuk binary'
SectionIn 1
SetOutPath $INSTDIR
File pavuk.exe
File pavuk-gtkrc
File COPYING
File AUTHORS
File CREDITS
SetOutPath $STARTMENU\Programs\Pavuk\
CreateShortCut "$STARTMENU\Programs\Pavuk\Pavuk uninstall.lnk" $INSTDIR\uninstall.exe "" "" 0
CreateShortCut "$STARTMENU\Programs\Pavuk\Pavuk.lnk" $INSTDIR\pavuk.exe "-x" "" 0

Section 'Cygwin runtime libraries'
SectionIn 1
SetOutPath $INSTDIR
File cygwin1.dll
File cygz.dll

Section 'Documentation'
SectionIn 1
SetOutPath $INSTDIR
File README
File NEWS
File TODO
File BUGS
File pavuk.1.html
SetOutPath $STARTMENU\Programs\Pavuk\
CreateShortCut "$STARTMENU\Programs\Pavuk\Pavuk commandline manual.lnk" $INSTDIR\pavuk.1.html "" "" 0

Section 'Internationalized message catalogs'
SectionIn 1
SetOutPath $INSTDIR\share\locale\sk\LC_MESSAGES
File sk\pavuk.mo
SetOutPath $INSTDIR\share\locale\cs\LC_MESSAGES
File cs\pavuk.mo
SetOutPath $INSTDIR\share\locale\de\LC_MESSAGES
File de\pavuk.mo
SetOutPath $INSTDIR\share\locale\es\LC_MESSAGES
File es\pavuk.mo
SetOutPath $INSTDIR\share\locale\it\LC_MESSAGES
File it\pavuk.mo
SetOutPath $INSTDIR\share\locale\fr\LC_MESSAGES
File fr\pavuk.mo
SetOutPath $INSTDIR\share\locale\ja\LC_MESSAGES
File ja\pavuk.mo

Section -post
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Stefan Ondrejicka\Pavuk" "Install Path" $INSTDIR
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pavuk" "DisplayName" "Pavuk (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pavuk" "UninstallString" '"$INSTDIR\uninstall.exe"'

Section Uninstall
DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pavuk"
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Stefan Ondrejicka\Pavuk"
Delete "$STARTMENU\Programs\Pavuk\Pavuk.lnk"
Delete "$STARTMENU\Programs\Pavuk\Pavuk commandline manual.lnk"
Delete "$STARTMENU\Programs\Pavuk\Pavuk uninstall.lnk"
RMDir $STARTMENU\Programs\Pavuk
Delete $INSTDIR\pavuk.exe
Delete $INSTDIR\pavuk-gtkrc
Delete $INSTDIR\cygwin1.dll
Delete $INSTDIR\cygz.dll
Delete $INSTDIR\README
Delete $INSTDIR\NEWS
Delete $INSTDIR\TODO
Delete $INSTDIR\BUGS
Delete $INSTDIR\COPYING
Delete $INSTDIR\AUTHORS
Delete $INSTDIR\CREDITS
Delete $INSTDIR\pavuk.1.html
Delete $INSTDIR\uninstall.exe
Delete $INSTDIR\share\locale\sk\LC_MESSAGES\pavuk.po
Delete $INSTDIR\share\locale\cs\LC_MESSAGES\pavuk.po
Delete $INSTDIR\share\locale\de\LC_MESSAGES\pavuk.po
Delete $INSTDIR\share\locale\es\LC_MESSAGES\pavuk.po
Delete $INSTDIR\share\locale\it\LC_MESSAGES\pavuk.po
Delete $INSTDIR\share\locale\fr\LC_MESSAGES\pavuk.po
Delete $INSTDIR\share\locale\ja\LC_MESSAGES\pavuk.po
RMDir $INSTDIR\share\locale\sk\LC_MESSAGES
RMDir $INSTDIR\share\locale\cs\LC_MESSAGES
RMDir $INSTDIR\share\locale\de\LC_MESSAGES
RMDir $INSTDIR\share\locale\es\LC_MESSAGES
RMDir $INSTDIR\share\locale\it\LC_MESSAGES
RMDir $INSTDIR\share\locale\fr\LC_MESSAGES
RMDir $INSTDIR\share\locale\ja\LC_MESSAGES
RMDir $INSTDIR\share\locale\sk
RMDir $INSTDIR\share\locale\cs
RMDir $INSTDIR\share\locale\de
RMDir $INSTDIR\share\locale\es
RMDir $INSTDIR\share\locale\it
RMDir $INSTDIR\share\locale\fr
RMDir $INSTDIR\share\locale\ja
RMDir $INSTDIR\share\locale
RMDir $INSTDIR\share
RMDir $INSTDIR
