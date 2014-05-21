# Microsoft Developer Studio Project File - Name="aacdec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=aacdec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aacdec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aacdec.mak" CFG="aacdec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aacdec - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "aacdec - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aacdec - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "release"
# PROP Intermediate_Dir "rel_obj"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\pub" /I "$(IPPROOT)/include" /D "NDEBUG" /D "REL_ENABLE_ASSERTS" /D "WIN32" /D "_MBCS" /D "_LIB" /D "USE_DEFAULT_STDLIB" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "aacdec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "debug"
# PROP Intermediate_Dir "debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\pub" /I "$(IPPROOT)/include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "USE_DEFAULT_STDLIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "aacdec - Win32 Release"
# Name "aacdec - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "general"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\aacdec.c
# End Source File
# Begin Source File

SOURCE=..\..\..\aactabs.c
# End Source File
# End Group
# Begin Group "ipp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\buffers.c
# End Source File
# Begin Source File

SOURCE=..\..\decelmnt.c
# End Source File
# Begin Source File

SOURCE=..\..\dequant.c
# End Source File
# Begin Source File

SOURCE=..\..\filefmt.c
# End Source File
# Begin Source File

SOURCE=..\..\imdct.c
# End Source File
# Begin Source File

SOURCE=..\..\noiseless.c
# End Source File
# Begin Source File

SOURCE=..\..\pns.c
# End Source File
# Begin Source File

SOURCE=..\..\stproc.c
# End Source File
# Begin Source File

SOURCE=..\..\tns.c
# End Source File
# End Group
# Begin Group "sbr_real"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\real\bitstream.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrfft.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrfreq.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrhfadj.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrhfgen.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrhuff.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrmath.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrqmf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrside.c
# End Source File
# Begin Source File

SOURCE=..\..\..\real\sbrtabs.c
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
