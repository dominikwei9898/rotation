# FITS-Client

This client implements the FITS protocol (as defined in EFI_FieryPath_FITS.pdf).  The purpose is to provide a starting point and reference when creating a FITS-Client.




### Table of contents

- Requirements
- Building/Running/Debugging (Visual Studio 2022) -- Windows binaries
- Building/Running/Debugging (Visual Studio 2022) -- Linux binaries
- Runtime Configuration




### Requirements

1. Visual Studio 2022 or later
2. Boost (version 1.83 or later)
3. Glog
4. Protobuf (version 2.5.0)
5. Libtiff (if building FITS-Output-Tiff (Linux))




### Building/Running/Debugging (Visual Studio 2022) -- Windows binaries

"FITS-Client_Windows" project (i.e. Windows x64 binaries):

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83):
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/windows.html to build and install.
    B. Glog:
        i. Follow instructions to download, build (potentially), and install at https://github.com/google/glog.
    C. Protobuf (version 2.5.0):
        i. Follow instructions to download, build, and install at https://github.com/protocolbuffers/protobuf/tree/v2.5.0.
    D. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Right-click the Windows "Start" icon, click "Search", enter "Edit the system environment variables", click on "Edit the system environment variables (Control Panel)".
    B. Click "Environment Variables" in the newly opened "System Properties" window:
        i.   Boost:
            a. Under "User variables for ...", click "New...".
            b. Under "Variable name" enter "MY_BOOST_DIR".
            c. Under "Variable value" enter the appropriate path to your Boost installation (e.g. ...\3rd_party\boost).
        ii.  Glog:
            a. Under "User variables for ...", click "New...".
            b. Under "Variable name" enter "MY_GLOG_DIR".
            c. Under "Variable value" enter the appropriate path to your Glog installation (e.g. ...\3rd_party\glog).
        iii. Protobuf:
            a. Under "User variables for ...", click "New...".
            b. Under "Variable name" enter "MY_PROTOBUF_DIR".
            c. Under "Variable value" enter the appropriate path to your Protobuf installation (e.g. ...\3rd_party\protobuf).
3. Update "FITS-Client_Windows" project in "FITS-Client.sln":
    A. You may need to adjust certain paths in the project configuration to match the directory structure of the headers and library files specific to your installation:
        i.  Adjust "include directories" under "FITS-Client_Windows -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii. Adjust "library directories" under "FITS-Client_Windows -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
4. Build "FITS-Client_Windows".


"FITS-Module_Windows" project (i.e. Windows x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Windows", you may skip the instructions under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83):
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/windows.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Right-click the Windows "Start" icon, click "Search", enter "Edit the system environment variables", click on "Edit the system environment variables (Control Panel)".
    B. Click "Environment Variables" in the newly opened "System Properties" window:
        i.  Boost:
            a. Under "User variables for ...", click "New...".
            b. Under "Variable name" enter "MY_BOOST_DIR".
            c. Under "Variable value" enter the appropriate path to your Boost installation (e.g. ...\3rd_party\boost).
3. Update "FITS-Module_Windows" project in "FITS-Client.sln":
    A. You may need to adjust certain paths in the project configuration to match the directory structure of the headers and library files specific to your installation:
        i.  Adjust "include directories" under "FITS-Module_Windows -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii. Adjust "library directories" under "FITS-Module_Windows -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
    B. Install Libtiff libraries:
        i.  Tools -> NuGet Package Manager -> Manage NuGet Packages for Solution.
        ii. Click "Restore" button in the right-side of the yellow area within the "NuGet - Solution" tab.
4. Build "FITS-Module_Windows".


"FITS-Output-Raw_Windows" project (i.e. Windows x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Windows" or "FITS-Module_Windows", you may skip the instructions under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83):
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/windows.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Right-click the Windows "Start" icon, click "Search", enter "Edit the system environment variables", click on "Edit the system environment variables (Control Panel)".
    B. Click "Environment Variables" in the newly opened "System Properties" window:
        i.  Boost:
            a. Under "User variables for ...", click "New...".
            b. Under "Variable name" enter "MY_BOOST_DIR".
            c. Under "Variable value" enter the appropriate path to your Boost installation (e.g. ...\3rd_party\boost).
3. Update "FITS-Output-Raw_Windows" project in "FITS-Client.sln":
    A. You may need to adjust certain paths in the project configuration to match the directory structure of the headers and library files specific to your installation:
        i.  Adjust "include directories" under "FITS-Output-Raw_Windows -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii. Adjust "library directories" under "FITS-Output-Raw_Windows -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
4. Build "FITS-Output-Raw_Windows".


"FITS-Output-Tiff_Windows" project (i.e. Windows x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Windows" or "FITS-Module_Windows", you may skip the instructions under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83):
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/windows.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Right-click the Windows "Start" icon, click "Search", enter "Edit the system environment variables", click on "Edit the system environment variables (Control Panel)".
    B. Click "Environment Variables" in the newly opened "System Properties" window:
        i.  Boost:
            a. Under "User variables for ...", click "New...".
            b. Under "Variable name" enter "MY_BOOST_DIR".
            c. Under "Variable value" enter the appropriate path to your Boost installation (e.g. ...\3rd_party\boost).
3. Update "FITS-Output-Tiff_Windows" project in "FITS-Client.sln":
    A. You may need to adjust certain paths in the project configuration to match the directory structure of the headers and library files specific to your installation:
        i.  Adjust "include directories" under "FITS-Output-Tiff_Windows -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii. Adjust "library directories" under "FITS-Output-Tiff_Windows -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
    B. Install Libtiff libraries:
        i.  Tools -> NuGet Package Manager -> Manage NuGet Packages for Solution.
        ii. Click "Restore" button in the right-side of the yellow area within the "NuGet - Solution" tab.
4. Build "FITS-Output-Tiff_Windows".


"FITS-Output-Prn_Windows" project (i.e. Windows x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Windows" or "FITS-Module_Windows", you may skip the instructions under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83):
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/windows.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Right-click the Windows "Start" icon, click "Search", enter "Edit the system environment variables", click on "Edit the system environment variables (Control Panel)".
    B. Click "Environment Variables" in the newly opened "System Properties" window:
        i.  Boost:
            a. Under "User variables for ...", click "New...".
            b. Under "Variable name" enter "MY_BOOST_DIR".
            c. Under "Variable value" enter the appropriate path to your Boost installation (e.g. ...\3rd_party\boost).
3. Update "FITS-Output-Prn_Windows" project in "FITS-Client.sln":
    A. You may need to adjust certain paths in the project configuration to match the directory structure of the headers and library files specific to your installation:
        i.  Adjust "include directories" under "FITS-Output-Prn_Windows -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii. Adjust "library directories" under "FITS-Output-Prn_Windows -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
4. Build "FITS-Output-Prn_Windows".


"FITS-Output-Simulator_Windows" project (i.e. Windows x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Windows" or "FITS-Module_Windows", you may skip the instructions under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83):
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/windows.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Right-click the Windows "Start" icon, click "Search", enter "Edit the system environment variables", click on "Edit the system environment variables (Control Panel)".
    B. Click "Environment Variables" in the newly opened "System Properties" window:
        i.  Boost:
            a. Under "User variables for ...", click "New...".
            b. Under "Variable name" enter "MY_BOOST_DIR".
            c. Under "Variable value" enter the appropriate path to your Boost installation (e.g. ...\3rd_party\boost).
3. Update "FITS-Output-Simulator_Windows" project in "FITS-Client.sln":
    A. You may need to adjust certain paths in the project configuration to match the directory structure of the headers and library files specific to your installation:
        i.  Adjust "include directories" under "FITS-Output-Simulator_Windows -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii. Adjust "library directories" under "FITS-Output-Simulator_Windows -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
4. Build "FITS-Output-Simulator_Windows".




### Building/Running/Debugging (Visual Studio 2022) -- Linux binaries

"FITS-Client_Linux (Linux)" project (i.e. Linux x64 binaries):

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83): 
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/unix-variants.html to build and install.
    B. Glog:
        i. Follow instructions to download, build (potentially), and install at https://github.com/google/glog.
    C. Protobuf (version 2.5.0):
        i. Follow instructions to download, build, and install at https://github.com/protocolbuffers/protobuf/tree/v2.5.0.
    D. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Log-in to the Linux server you will use to build the binaries.
    B. Set environment variables (according to the syntax of your shell):
        i.   Boost (e.g. /3rd_party/boost).
        ii.  Glog (e.g. /3rd_party/glog).
        iii. Protobuf (e.g. /3rd_party/protobuf).
3. Update "FITS-Client_Linux (Linux)" project in "FITS-Client.sln":
    A. You may need to adjust project settings based on your 3rd party library versions and directory structures specific to your installations:
        i.   Adjust "include directories" under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii.  Adjust "library directories" under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
        iii. Adjust "library names" under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> Input -> Library Dependencies".
    B. Enter your Linux build server information:
        i.  Enter credentials under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Build Machine".
        ii. Enter build directory under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Root Directory".
4. Build "FITS-Client_Linux (Linux)".


"FITS-Module_Linux (Linux)" project (i.e. Linux x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Linux (Linux)", you may skip the instructions for "Boost" under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83): 
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/unix-variants.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Log-in to the Linux server you will use to build the binaries.
    B. Set environment variables (according to the syntax of your shell):
        i.  Boost (e.g. /3rd_party/boost).
3. Update "FITS-Module_Linux (Linux)" project in "FITS-Client.sln":
    A. You may need to adjust project settings based on your 3rd party library versions and directory structures specific to your installations:
        i.   Adjust "include directories" under "FITS-Module_Linux (Linux) -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii.  Adjust "library directories" under "FITS-Module_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
        iii. Adjust "library names" under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> Input -> Library Dependencies".
    B. Enter your Linux build server information:
        i.  Enter credentials under "FITS-Module_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Build Machine".
        ii. Enter build directory under "FITS-Module_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Root Directory".
4. Build "FITS-Module_Linux (Linux)".


"FITS-Output-Raw_Linux (Linux)" project (i.e. Linux x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Linux (Linux)", you may skip the instructions for "Boost" under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83): 
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/unix-variants.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Log-in to the Linux server you will use to build the binaries.
    B. Set environment variables (according to the syntax of your shell):
        i.  Boost (e.g. /3rd_party/boost).
3. Update "FITS-Output-Raw_Linux (Linux)" project in "FITS-Client.sln":
    A. You may need to adjust project settings based on your 3rd party library versions and directory structures specific to your installations:
        i.   Adjust "include directories" under "FITS-Output-Raw_Linux (Linux) -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii.  Adjust "library directories" under "FITS-Output-Raw_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
        iii. Adjust "library names" under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> Input -> Library Dependencies".
    B. Enter your Linux build server information:
        i.  Enter credentials under "FITS-Output-Raw_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Build Machine".
        ii. Enter build directory under "FITS-Output-Raw_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Root Directory".
4. Build "FITS-Output-Raw_Linux (Linux)".


"FITS-Output-Tiff_Linux (Linux)" project (i.e. Linux x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Linux (Linux)", you may skip the instructions for "Boost" under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83): 
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/unix-variants.html to build and install.
    B. Libtiff:
        i. Follow instructions to download, build, and install at https://www.linuxfromscratch.org/blfs/view/svn/general/libtiff.html.
    C. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Log-in to the Linux server you will use to build the binaries.
    B. Set environment variables (according to the syntax of your shell):
        i.  Boost (e.g. /3rd_party/boost).
        ii. Libtiff (e.g. /3rd_party/libtiff).
3. Update "FITS-Output-Tiff_Linux (Linux)" project in "FITS-Client.sln":
    A. You may need to adjust project settings based on your 3rd party library versions and directory structures specific to your installations:
        i.   Adjust "include directories" under "FITS-Output-Tiff_Linux (Linux) -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii.  Adjust "library directories" under "FITS-Output-Tiff_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
        iii. Adjust "library names" under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> Input -> Library Dependencies".
    B. Enter your Linux build server information:
        i.  Enter credentials under "FITS-Output-Tiff_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Build Machine".
        ii. Enter build directory under "FITS-Output-Tiff_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Root Directory".
4. Build "FITS-Output-Tiff_Linux (Linux)".


"FITS-Output-Prn_Linux (Linux)" project (i.e. Linux x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Linux (Linux)", you may skip the instructions for "Boost" under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83): 
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/unix-variants.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Log-in to the Linux server you will use to build the binaries.
    B. Set environment variables (according to the syntax of your shell):
        i.  Boost (e.g. /3rd_party/boost).
3. Update "FITS-Output-Prn_Linux (Linux)" project in "FITS-Client.sln":
    A. You may need to adjust project settings based on your 3rd party library versions and directory structures specific to your installations:
        i.   Adjust "include directories" under "FITS-Output-Prn_Linux (Linux) -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii.  Adjust "library directories" under "FITS-Output-Prn_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
        iii. Adjust "library names" under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> Input -> Library Dependencies".
    B. Enter your Linux build server information:
        i.  Enter credentials under "FITS-Output-Prn_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Build Machine".
        ii. Enter build directory under "FITS-Output-Prn_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Root Directory".
4. Build "FITS-Output-Prn_Linux (Linux)".


"FITS-Output-Simulator_Linux (Linux)" project (i.e. Linux x64 binaries):
*** NOTE: If you have already followed the steps above for "FITS-Client_Linux (Linux)", you may skip the instructions for "Boost" under Step "1" and "2" below. ***

1. Download, build, and install the required 3rd party libraries:
    A. Boost (version 1.83): 
        i.  Download boost_1_83_0.zip at https://www.boost.org/users/history/version_1_83_0.html.
        ii. Follow instructions at https://www.boost.org/doc/libs/1_83_0/more/getting_started/unix-variants.html to build and install.
    B. If having trouble with the 3rd party libraries, contact the Fiery team and we may be able to provide a package with the required 3rd party libraries.
2. Create environment variables:
    A. Log-in to the Linux server you will use to build the binaries.
    B. Set environment variables (according to the syntax of your shell):
        i.  Boost (e.g. /3rd_party/boost).
3. Update "FITS-Output-Simulator_Linux (Linux)" project in "FITS-Client.sln":
    A. You may need to adjust project settings based on your 3rd party library versions and directory structures specific to your installations:
        i.   Adjust "include directories" under "FITS-Output-Simulator_Linux (Linux) -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories".
        ii.  Adjust "library directories" under "FITS-Output-Simulator_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories".
        iii. Adjust "library names" under "FITS-Client_Linux (Linux) -> Properties -> Configuration Properties -> Linker -> Input -> Library Dependencies".
    B. Enter your Linux build server information:
        i.  Enter credentials under "FITS-Output-Simulator_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Build Machine".
        ii. Enter build directory under "FITS-Output-Simulator_Linux (Linux) -> Properties -> Configuration Properties -> General -> Remote Root Directory".
4. Build "FITS-Output-Simulator_Linux (Linux)".




### Runtime Configuration

1. The file module/common/configuration/FITS-Module.json contains the settings to configure/control runtime behavior
    A.  The element "fits_server.ip" sets the IP of the FITS-Server the given FITS-Module instance will attempt to connect to.
    B.  The element "logging.console" controls whether the logging module will log messages to the FITS-Client's console window.
        i. This only applies to log messages that are invoked with the "console_log" parameter set to "true".
    C.  The element "colorant.buffer_pool_size_in_megabytes" controls the size of the buffer used to store image/raster data received from the FITS-Server.
        i. This value is used to allocate a buffer for each colorant supported by the given FITS-Module instance (based on the configuration of the FITS-Server connected).
    D.  The elements "fiery_path_client.ip" and "fiery_path_client.port" sets the IP and port of the FieryPath-Client the given FITS-Module instance will attempt to connect to.
    E.  The element "output.plugin" controls the FITS-Output plug-in that is loaded.
        i. The "default" FITS-Output plug-in is FITS-Output-Raw.
    F.  The elements "output_raw.file_enabled" and "output_raw.job_name_enabled" control the FITS-Output-Raw features.
        i.   If "output_raw.file_enabled" is enabled, the "raw" image data will be written to a "raw" file.
        ii.  If "output_raw.job_name_enabled" is enabled (and "output_raw.file_enabled" is enabled), the "raw" filename will include/use the Fiery's job name.
    G.  The elements "output_tiff.file_enabled" and "output_tiff.job_name_enabled" control the FITS-Output-Tiff features.
        i.  If "output_tiff.file_enabled" is enabled, the "tiff" data will be written to a "tiff" file.
        ii. If "output_tiff.job_name_enabled" is enabled (and "output_tiff.file_enabled" is enabled), the "tiff" filename will include/use the Fiery's job name.
    H.  The elements "output_prn.file_enabled" and "output_prn.job_name_enabled" control the FITS-Output-Prn features.
        i.  If "output_prn.file_enabled" is enabled, the "prn" data will be written to a "prn" file.
        ii. If "output_prn.job_name_enabled" is enabled (and "output_prn.file_enabled" is enabled), the "prn" filename will include/use the Fiery's job name.
    I.  The elements "output_simulator.transfer_time_in_milliseconds" and "output_simulator.images_per_minute" control the Press "simulator" feature.
        i.  The element "output_simulator.transfer_time_in_milliseconds" attempts to "simulate" the image/raster "transfer" time to the Press by "sleeping" for the specified time (in milliseconds).
        ii. The element "output_simulator.images_per_minute" attempts to "simulate" the image/page "ejected" rate from the Press by "sleeping" for an appropriate amount of time.