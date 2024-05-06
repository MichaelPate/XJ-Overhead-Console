# XJ-Overhead-Console
Overhead Console Trip Computer for a 1990 Jeep Cherokee XJ. The goal is to display things like gyroscope readings and log GPS points.

The overhead console, or trip computer, should display the folling data by the time its fully complete:
- RTC time and date
- Temperature, pressure, humidity, both inside/outside
-   A remote mount sensor would be needed, with likely wireless radio communications
- Compass heading (cardinal and degrees)
- GPS latitude and longitude
- GPS speed (if accurate enough)
- GPS distance (calculated from sum of prior points or line-of-sight)
- Gyroscope roll and pitch
-   Gyroscope should be able to be zeroed
- Accelerometer readings that are above a threshold
-   Threshold should be settable


The trip computer should be able to control the folling
- Off road lights (through relays)
-   User control of lights acheived through toggle switches
-   The ability to control off road lights gives the ability to alarm
- The horn (through a relay)
-   User control of horn acheived through pull cord mounted on side of head console
-   The ability to control the horn gives the ability to alarm
- A GSM module capable of sending SMS messages
-   This allows for GPS location in the event of a theft
-   Or possibly sending SOS messages to contacts in the event of an emergency
-   Or possibly remote kill switch capability

The trip computer should have the ability to connect to an SD card or USB flash drive:
- This allows for datalogging of all sensor data for later viewing
- Control of datalogging would be minimal. Auto generated file names.
-   User control of datalogging would be through a push button or toggle switch

A possible feature would be alarming. The alarm state could be acheived
through significant detection of motion, a GPS reading outside a geo-fence, or a combination
The addition of a PIR sensor inside the cab is possible, if experimentation shows it to not be too sensetive to activity outside the car.
