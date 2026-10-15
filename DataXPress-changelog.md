# Change Log
All notable changes for DataXPress Fiery Path and FITS client samples will be documented in this file.

## 1.0.6 - 2025-04-07

### Changes
* Added error handling when attempting to create directories in the various output modules of FITS-Client.

## 1.0.5 - 2025-03-14

### Changes
* Added "version" to FITS-Client log.
* Updated Visual Studio projects, added "static_cast", and re-organized include paths to eliminate several compilation "warnings".
* Ported condition variables, mutexes, and threads from Boost to std to reduce the number of dependencies on Boost.

## 1.0.4 - 2024-11-21

### Changes
* Close the window where a login can be initiated when one is already in progress

## 1.0.3 - 2024-05-24

### Changes
* Fixed filename issue related to a directory bug with FITS-Output-Tiff

## 1.0.2 - 2024-05-15

### Changes
* Fixed header format issue with FITS-Output-Prn

## 1.0.1 - 2024-05-01

### Changes
* Initial release
