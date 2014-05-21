                                                       ___            
         .-.     S i Z i O U S  P R E S E N T S . . . (   )           
        ( __)  ___ .-. .-.     .--.        ,--.     .-.| |    .--.    
        (''") (   )   '   \   /    \      /   |    /   \ |   /    \   
         | |   |  .-.  .-. ; ;  ,-. '    / .' |   |  .-. |  |  .-. ;  
         | |   | |  | |  | | | |  | |   / / | |   | |  | |  |  |(___)        
         | |   | |  | |  | | | |  | |  / /  | |   | |  | |  |  |      
         | |   | |  | |  | | | |  | | /  `--' |-. | |  | |  |  | ___  
         | |   | |  | |  | | | '  | | `-----| |-' | '  | |  |  '(   ) 
         | |   | |  | |  | | '  `-' |       | |   ' `-'  /  '  `-' |  
        (___) (___)(___)(___) `.__. |      (___)   `.__,'    `.__,'   
                              ( `-' ;           i M G 4 D C                           
                               `.__.     Dreamcast Selfboot Toolkit                             
					
		   	
       After 6 years of idle time on my Dreamcast projects, I decided to 
       release my favorites Dreamcast tools source, mds4dc and cdi4dc, 
       grouped in one single package: Ladies and Gentlemens, I'm very 
       proud to introduce you IMG4DC, the ultimate Dreamcast Selfboot 
       Toolkit package!	

      ____                      _        __  _            
     / __ \___  _______________(_)____  / /_(_)____  ____ 
    / / / / _ \/ ___/ ___/ ___/ // __ \/ __/ // __ \/ __ \
___/ /_/ /  __(__  ) /__/ /  / // /_/ / /_/ // /_/ / / / /_____________________
  /_____/\___/____/\___/_/  /_// .___/\__/_/ \____/_/ /_/ I. DESCRiPTiON
                              /_/                       
    
    This project contains two masterpieces of Dreamcast Selfboot tools, 
    capable of generating *VALiD* burnable disc images in the followings 
    formats:
	                  Alcohol 120% (MDS/MDF)
                          Padus DiscJuggler (CDI)

    By saying *VALiD* images, it refers that the disc images are readable by 
    mounting theses under a virtual drive, like Daemon Tools or Alcohol 
    software. You'll be able to test your Dreamcast selfboot projects in
    real time, and allows the end-user use your Dreamcast production as the
    easier and faster way.

    Theses tools are currently used in anothers projects, like BootDreams or 
    Selfboot Inducer, for at least 5 years, it proves the software quality.

    No improvements was made on the sources released here. No, I'm not given 
    up the Dreamcast. I'm just releasing these because I haven't much time now
    to improve the tools like I want. So I'm giving theses tools to the
    Dreamcast communauty, with the hope that YOU can improve them.
	
      ______                       _ ___             
     / ____/____  ____ ___  ____  (_) (_)____  ____ _
    / /    / __ \/ __ `__ \/ __ \/ / / // __ \/ __ `/
___/ /___ / /_/ / / / / / / /_/ / / / // / / / /_/ /___________________________
   \____/ \____/_/ /_/ /_/ .___/_/_/_//_/ /_/\__, / II. COMPiLiNG
                        /_/                 /____/   

    To compile this package, you must have the MinGW package installed on your
    computer. I made a few changes in the source code to allow the source
    compiling under the lastest MinGW release (mingw-get-inst-20120426).

    Please add the "C:\MinGW\bin\" path to your PATH environment variable
    after the MinGW installation (adapt the MinGW root path as needed).
						
:: II.1 Compiling libedc ::::::::::::::::::::::::::::::::::::::::::::::::::::::

    The libedc library was ripped from the cdrtools package. It isn't the 
    lastest release, so I think this part can be updated.

    This library is used both by the cdi4dc and mds4dc projects, so you need 
    to compile this component before starting anything.

    To compile it, just double-click on the 'build.bat' file inside the
    ".\libedc\src" directory.

    After that, you should have the ".\libedc\lib\libedc.a" file created.

:: II.2 Compiling cdi4dc ::::::::::::::::::::::::::::::::::::::::::::::::::::::
						
    Every selfboot tool inside this package can be compiled now. First we'll
    start with cdi4dc which is the older selfboot tool but you can compile 
    mds4dc like the same way if you want.

    To compile cdi4dc, just double-click on the 'build.bat' inside the 
    ".\cdi4dc\" directory. That's it.

    After that, you should have the ".\cdi4dc\bin\cdi4dc.exe" file. Just zip
    the bin directory (with the "docs" folder) to make a release.

    If you want, you can active the UPX (Ultimate Packer for Executable) after
    the cdi4dc compilation. Edit the Makefile and edit it in consequence.

:: II.3 Compiling mds4dc ::::::::::::::::::::::::::::::::::::::::::::::::::::::

    For compiling mds4dc, it's just like cdi4dc. Go to the ".\mds4dc" directory
    and double-click on the 'build.bat' file.

    Don't forget to include lbacalc binary (keep reading this document).

:: II.4 Compiling lbacalc :::::::::::::::::::::::::::::::::::::::::::::::::::::

    For compiling lbacalc, needed by mds4dc to compute the msinfo value when
    using the tool with CDDA tracks, it's just like the other tools.
    Go to the ".\lbacalc" directory and double-click on the 'build.bat' file.

:: II.5 Before releasing ::::::::::::::::::::::::::::::::::::::::::::::::::::::

    If you made significants changes and join the MDS4DC developers, just mail
    me at sizious (at) gmail (dot) com. I'll add you with pleasure.

    Don't forget to include the 'docs' directory inside each 'bin' directory
    as it contains licenses and documentation material.

    And of course, don't forget to edit the 'version.rc' file which contains
    the version information and other things.
       __    _                          
      / /   (_)________  ____  ________ 
     / /   / // ___/ _ \/ __ \/ ___/ _ \
___ / /___/ // /__/  __/ / / (__  )  __/_______________________________________
   /_____/_/ \___/\___/_/ /_/____/\___/ III. LiCENSE
                                     
    This package is released under the GNU GPL 3.

    So please keep it mind by using it.					
						
      ______               ___ __      
     / ____/________  ____/ (_) /______
    / /    / ___/ _ \/ __  / / __/ ___/
___/ /___ / /  /  __/ /_/ / / /_(__  )_________________________________________
   \____//_/   \___/\__,_/_/\__/____/ IV. CREDiTS
   
    Each source code was written by [big_fury]SiZiOUS (SiZiOUS or simply 
    'SiZ!'), except the "libedc" library of course.
   
    Credits flying to (in no order) :
    BlackAura, Fackue, Xeal, DeXT, Heiko Eissfeldt, Joerg Schilling, Henrik 
    Stokseth, Marcus Comstedt, Ron, JMD, speud and all the rest I forgot.

    Enjoy with this cool project.

    Your truely,
    
    SiZiOUS
    http://sbibuilder.shorturl.com/

    Check my other SourceForge Project:
    Shenmue Translation Pack - http://shenmuesubs.sourceforge.net/

< EOF 09-May-2012 >
