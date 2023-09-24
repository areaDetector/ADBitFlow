ADSBitFlow Releases
==================

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADBitFlow.

Tagged source code releases can be obtained at
https://github.com/areaDetector/ADBitFlow/releases.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============

R1-0 (September XXX, 2023)
-------------------
* Initial release.
The following things need to be fixed before the first release:
  - Control of Height and Width
  - Control of pixel depth
  - Eliminate data copying from frame buffers?
  - Prevent more frames being processed after Acquire set to 0?
  - Why does camera start acquiring as soon as application starts?  Ximilon does not.
  - Why is sleep needed when stopping camera before AcquisitionStop?
  - Build trigger cable, test trigger modes
  - Ask BitFlow support
    - What versions of Linux are supported
    - How to set an ROI and increase frame rate without loading a different camera file?
    - How to enable 10-bit mode
    - Why is HighResTimeStamp 0 in all fields
    - Why is a sleep needed when stopping camera before AcquisitionStop
    - AcquisitionFrameRate=2 works fine, 1 stops after 5-10 frames with Circular acquisition aborted" in waitDoneFrame.  Why. 
      I tried setting setTimeout=2000000 in constructor but that did not help.


