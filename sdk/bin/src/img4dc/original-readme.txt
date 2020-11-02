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
      * Alcohol 120% (MDS/MDF)
      * Padus DiscJuggler (CDI)

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

    This project is assuming that you're using a 64-bit Windows. If not the
    case, please read the note at the end of this chapter.

    In order to compile, you must have several packages installed on your 
    computer:
      1. TDM-GCC-64  : tdm-gcc.tdragon.net
      2. UPX         : upx.sourceforge.net
      3. CodeLite IDE: www.codelite.org

    Install all packages above in the specified order.
	
    When installing UPX (Ultimate Packer for Executable), please add it to 
    your PATH environment variable, as it'll be used when compiling IMG4DC
    in Release mode.
	
    NOTE: To compile this project, the prerequisite is to have a 64-bit Windows. 
    If not the case, you'll need to adapt the CodeLite project settings in order 
    to use the TDM-GCC-32 toolchain instead. Currently, binaries produced are
    in 32-bit mode, but it's time to plan to switch to 64-bit in the future...
	
    After the installation, you'll be able to compile the IMG4DC package.
	
    Start CodeLite, then configure the TDM-GCC-64 toolchain if you don't have
    already done it. Then open the 'img4dc.workspace' file by using the 
    Workspace > Open Workspace... command inside CodeLite.
	
    Everything was already configured to compile the whole workspace in the
    correct way. Just choose the build configuration ('Debug' or 'Release') in 
    the Workspace View (in the left by default), right-click on the 'img4dc'
    root node and select 'Build Workspace'.
	
:: II.1 Compiling libraries ::::::::::::::::::::::::::::::::::::::::::::::::::::

    Before starting anything, you must compile the 2 provided libraries, 
    which is 'edc' and 'common'. These libraries are used in the whole project, 
    so you need to compile them before doing everything. This is already done
    automatically by CodeLite.

    The libedc ('edc') library was ripped from the cdrtools package. It isn't 
    the latest release, so I think this part can be updated. It's responsible
    for computing ECC/EDC data sectors.
	
    The libcommon ('common') library contains some useful functions available
    in the projects.

:: II.2 Compiling cdi4dc ::::::::::::::::::::::::::::::::::::::::::::::::::::::
					
    To compile cdi4dc, just select the Release mode in CodeLite then the build
    command.

    After that, you should have the '.\cdi4dc\bin\Release\cdi4dc.exe' file. 
    Just zip the bin directory to make a release. Don't forget to include the
    'doc' folder.

:: II.3 Compiling mds4dc ::::::::::::::::::::::::::::::::::::::::::::::::::::::

    For compiling mds4dc, it's just like cdi4dc. Select the Release mode in
    CodeLite then the build command.

    Don't forget to include lbacalc binary (keep reading this document).

:: II.4 Compiling lbacalc :::::::::::::::::::::::::::::::::::::::::::::::::::::

    For compiling lbacalc, needed by mds4dc to compute the msinfo value when
    using the tool with CDDA tracks, it's just like the other tools.
    
    Select the Release mode for lbacalc within CodeLite then the build command.

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
   
    Each source code was written by SiZiOUS or simply 'SiZ!'), except the 
    'libedc' library of course.
   
    Credits flying to (in no order) :
    BlackAura, Fackue, Xeal, DeXT, Heiko Eissfeldt, Joerg Schilling, Henrik 
    Stokseth, Marcus Comstedt, Ron, JMD, speud, IlDucci, PacoChan and all the
    rest I forgot.

    Enjoy this cool project.

    Your truely,
    SiZiOUS
    
    *** Wanna contact me? ***
	
    www : www.sizious.com
    mail: sizious (at) gmail (dot) com
    fb  : fb.com/sizious
    tw  : @sizious
______________________________________________________________/ EOF 20150812 /__
