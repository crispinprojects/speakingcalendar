# Talk Calendar

Talk Calendar is a personal desktop calendar for Linux which has some speech capability using its own built-in text-to-speech synthesizer. It can read out current day events and upcoming events (if required) at start up.

It has been developed using C and [GTK4](https://docs.gtk.org/gtk4/). A screenshot is shown below.

![](talkcalendar.png)

Time and events are what drives life.

## Core Features

* built with C and GTK 4.14.4 (Fedora 40)
* month-view calendar 
* event details, location, start and end time can be entered and edited
* export and import iCalendar files (backup and restore)
* event time reminder alarms (sent as system notifications and audio alert)
* Sqlite3 database used to store events
* built-in diphone speech synthesizer

### Local Install Using Prebuilt Binary

A 64-bit prebuilt binary for the latest version of Talk Calendar is available and can be downloaded from the binary directory. This has been built using GTK 4.14 and tested with Fedora 40 and Ubuntu 24.04 LTS. 

Extract the downloaded file which contains the Talk Calendar executable. Assuming that the GTK4 base libraries are installed the Talk Calendar binary can be run from the terminal using:

```
./talkcalendar
```

or double click on the "talkcalendar" file. Talk Calendar must have executable permissions to execute. Right click it and choose Properties->Permissions and tick allow "Executable as Program".

To add Talk Calendar to the system menu modify the Talk Calendar desktop file provided in the download. A desktop file has a .desktop extension and provides metadata about an application such as its name, icon, command to execute and other properties. For user-specific applications desktop files can be located locally in the ~/.local/share/applications/ directory. Local user entries take precedence over system entries. For the GNOME desktop, the desktop file should be named using the [application ID](https://developer.gnome.org/documentation/tutorials/application-id.html), that is <application_id>.desktop, which in this case is "org.gtk.talkcalendar.desktop" 

The "org.gtk.talkcalendar.desktop" file is shown below. You need to modify this using your own user name and directory locations. For example, if your user name is "tiki" and you install local applications in a folder called "Software" and you create a folder called "talkcalendar " to store the Talk Calendar binary executable then the executable path would be "Exec=/home/tiki/Software/talkcalendar/talkcalendar". The Exec variable defines the command to execute when launching an application, in this case, the talkcalendar binary executable. The Path variable tells the system where to look for the executable and the calendar database. The Icon variable specifies the path to the icon file associated with application. In a .desktop file, you need to use absolute and full paths.

```
[Desktop Entry]
Version=0.3.0
Type=Application
Name=Talk Calendar
Comment=Talking calendar
Icon=/home/your-user-name/folder/talkcalendar/calendar.png
Exec=/home/your-user-name/folder/talkcalendar/talkcalendar
Path=/home/your-user-name/folder/talkcalendar
X-GNOME-UsesNotifications=true
Categories=Calendar;Office;
MimeType=text/calendar;
```

Copy your modified  "org.gtk.talkcalendar.desktop" file to the ***~/.local/share/applications/***  hiden directory (tick the "Show Hidden Files" option in the file explorer). Create the ~/.local/share/applications/ directory if it does not already exist. This way of locally installing Talk Calendar should be universal across different Linux distributions.

## Calendar Usage

If you have used a calendar application before then using Talk Calendar will be straight forward. 

### Adding New Event

* Click on the new event button in the header bar or press Ctrl+n to invoke the "New Event" window
* Enter the event summary (e.g. birthday)
* Enter the description 
* Enter the location
* Enter the start date by setting the day, month and year values 
* Enter start and end times (or tick the all day check box)
* Events are sorted by start time when displayed
* Check the "Is Yearly" check box if the event repeats every year (e.g. birthdays and anniversaries)

A screenshot of the new event dialog is shown below.

![](talkcalendar-new-event.png)

### Reminders

Talk Calendar supports the use of reminders. A reminders is an alarm triggered at a specified time before an event starts.

* Check the "Reminder" check box and set the reminder time (hour, minute) which should be a time before the event starts.

When a reminder is triggered a notification is sent to the system and an audio message played

![](talkcalendar-reminder.png)

### Display and Speak Events

* Click on a date in the month-view calendar which has events and this will invoke the day events list view as shown below.

![](talkcalendar-day-events.png)

* Event details are spoken when the day events list view is invoked (assuming that talking is selected in the preferences).

* The day events list view allows individual events to be selected so that they can be edited or deleted (make sure you click on the event before using either the "Edit Event" or "Delete Event" button).

* Talk Calendar also supports tooltips so that if you hover over a date in the month-view calendar the event details are displayed.

### Searching For Events

* Select "Search For Events" from the hamburger menu .
* Enter a search term or location.

![](talkcalendar-search.png)

![](talkcalendar-search-results.png)

### Preferences

* Select "Preferences" from the hamburger menu or use Ctrl+Alt+p to invoke the preferences window (see screenshot below).
* Change options as required.

![](talkcalendar-preferences.png)

* Check "Show Public Holidays" to display the main UK public holidays on the Calendar.
* Click on the colour buttons to change the today, event-day and public holiday calendar colours.

![](talkcalendar-colour-dialog.png)

## Talking

* Events are spoken when a day in the month-view calendar is selected (assuming that talking is selected in the preferences).
* Speaking the selected day events can also be done by pressing the "s" key or using the menu item "Speak Selected Day".
* Talk Calendar will tell you the time if you press the "t" key.
* Change the "Talk Rate" value in the Preferences dialog to speed up talking. (if required)

### Information

* Select "Information"  from the hamburger menu or press F1
* the information window shows the keyboard shoutcuts, how many records are in the database, the Sqlite version being used on the system, the desktop font and scale factor.

![](talkcalendar-information.png)

* Use the About dialog to display current version.

![](talkcalendar-about.png)

## Startup Applications

With GNOME based desktops you can use the GNOME "Tweak Tool" to add Talk Calendar to your startup applications. Talk Calendar will then read out the current date and days events and any future upcoming events (see preferences settings) when the computer is switched on.

### Events Database

Events are stored in an [Sqlite](https://www.sqlite.org/index.html) database. SQLite is a small, fast and full-featured SQL database engine written in C. 

### Export and Import iCalendar Files

Talk Calendar allows a personal calendar to be exported as an iCalendar file. These typically use the file extension ".ical" or ".ics". The [iCalendar standard](https://icalendar.org/) is an open standard for exchanging calendar and scheduling information between users and computers.  An icalendar file is a plain text file and so can be modified using a standard text editor. 

The export to icalendar file does not currently support time zones and so the DTSTART and DTEND properties contain dates with local time and have no reference to a time zone. For example, the following represents an event starting on January, 1st, 2024 at 11.30am and ending at 2pm.

```
DTSTART:20240101T113000
DTEND:20240101T140000
```

A file chooser dialog is used to allow the save directory location to be chosen by the user as shown below.

![](talkcalendar-export-ical.png)

The icalendar import parser allows the date and local time to be imported and checks if a time zone has been specified using the [TZID](https://icalendar.org/iCalendar-RFC-5545/3-2-19-time-zone-identifier.html) property. A file chooser dialog is used to allow the file to be chosen by the user as shown below. File filters can be used.

![](talkcalendar-import-ical.png)

The parser will be updated with new features in future releases.

### Recurring Events

The only recurring event type that is currently supported by Talk Calendar is yearly. This is required for events such as birthdays and anniversaries. The parser uses icalendar [RRULE](https://icalendar.org/iCalendar-RFC-5545/3-8-5-3-recurrence-rule.html) to determine if an event is yearly (e.g. birthday).

### Upgrading To Newer version

It is strongly suggested that you save your current calendar in "ical" format before updating to a newer version of Talk Calendar. Install Talk Calendar with a new "calendar.db" database and import the ical file to this.

## Build From Source

The C source code for the Talk Calendar application is provided in the src directory. Fedora 40 has been used to develop and compile the project. Fedora 40 uses GTK 4.14.

[Geany](https://www.geany.org/) can be used as a source code editor for opening, viewing and then compiling the Talk Calendar C code. Geany is lightweight and has an integrated terminal for building the application.

To build Talk Calendar from source you need the gcc compiler, GTK4, GLIB and SQLITE  development libraries. 

### Building on Fedora 40

With Fedora you need to install the following packages to compile Talk Calendar.

```
sudo dnf install gcc make
sudo dnf install gtk4-devel
sudo dnf install gtk4-devel-docs
sudo dnf install glib-devel
sudo dnf install alsa-lib-devel
sudo dnf install sqlite-devel
sudo dnf install libnotify-devel
```

To check the installed Sqlite 3 version use the command below.

```
sqlite3 --version
```

To check the installed version of the GTK4 development libraries use the command below.

```
dnf list gtk4-devel
```

Use the MAKEFILE to compile. 

```
make
```

To run Talk Calendar from the terminal use

```
./talkcalendar
```

### Building on Ubuntu 24.04 LTS

With Ubuntu and you need to install the following packages to compile Talk Calendar.

```
sudo apt install build-essential
sudo apt install libgtk-4-dev
sudo apt install libasound2-dev
sudo apt install sqlite3
sudo apt install libsqlite3-dev
sudo apt install libnotify-dev
```

The packages:

```
apt install libglib2.0-dev
apt install alsa-utils
```

are needed but should be installed by default. 

You may need to use the [Ubuntu snap store](https://snapcraft.io/) to install things like Geany.

### Building on Debian 12 Bookworm

Debian 12 Bookworm uses GTK4.8. The Talk Calendar source code has been developed using GTK4.14 (Fedora 40) and so it will not compile using GTK4.8 without making a number of changes (see code notes below). Debian testing (Trixie) is the current development state of the next stable Debian distribution. According to the [Debian GTK4 tracker](https://tracker.debian.org/pkg/gtk4) Trixie currently has GTK4.12 in its repositories. Debian experimental is using GTK4.14. Talk Calendar code will most likely compile using Debian testing and experimental but I have not tried this.

To determine which version of GTK4 is running on a Debian system use the following terminal command.

```
dpkg -l | grep libgtk*
```

## Code Notes

I am now using Fedora 40 to develop the Talk Calendar application and not Debian 12 Bookworm. Fedora 40 uses GTK4.14 as opposed to the older GTK 4.8 used by Debian 12 Bookworm. This mean that the code will not compile with Debian 12 Bookworm without making changes to the source code. For example, Talk Calendar now uses "gtk_css_provider_load_from_string" rather than "gtk_css_provider_load_from_data" which was depreciated in GTK 4.12.

A bigger change is that the GtkFileDialog API is no longer signal based. With GTK4.12 and above it is callback based which should match a GAsyncReadyCallback function (async/await pattern). In computer programming, the async/await pattern is a syntactic feature that allows an asynchronous, non-blocking function to be structured in a way similar to an ordinary synchronous function. With my Debian 12 (GTK4.8) projects I used the older function "gtk_file_chooser_dialog_new" with a response callback but this approach has been depreciated.

Code related to colours has been migrated to use GdkRGBA as GdkColor is being deprecated. The code for setting HTML CSS colour names has been completely removed in favour of the new GtkColorDialog API. A [GtkColorDialogButton](https://docs.gtk.org/gtk4/class.ColorDialogButton.html) is wrapped around a GtkColorDialog allowing a colour chooser dialog to be opened to change calendar colours.

Playing audio using GThread and GMutex has been replaced with GTask (async/wait pattern). With GTK4 it appears that the preferred way to perform work in a thread is to use GTask. The code now uses [g_task_run_in_thread()](https://docs.gtk.org/gio/method.Task.run_in_thread.html) so that a play audio blocking operation is executed in a separate background thread. The function g_task_run_in_thread() turns a synchronous operation into an asynchronous one, by running it in a thread. Apparently, GTask maintains a thread pool that is based on the number of CPUs available (i.e. supports multiple CPU-cores). The basic GTask code structure is now working but will be updated as I learn more. I have now added a guard to prevent multiple audio clips being played at once.

The Calendar itself has received a number of updates including using tooltips to show event details when you hover over a day in the month-view display.

## Speech Synthesis

Talk Calendar uses its own built-in diphone speech synthesizer.  A diphone is a sound unit composed of two adjacent partial phonemes i.e. the second half of the first phoneme and the first half of the second phoneme. Words are formed as sequences of elementary speech units called phonemes. A phoneme is the smallest unit of sound that distinguishes one word from another word and there are 44 phonemes in the English language. The synthesizer uses a set of pre-recorded diphone sound samples and concatenates these to produce synthesized speech output for a given text input.

The voice used by Talk Calendar is based on the diphone collection created by Alan W Black and Kevin Lenzo which is free for use for any purpose (commercial or otherwise) and subject to the pretty light restrictions [detailed here](https://github.com/hypnaceae/DiphoneSynth/blob/master/diphones_license.txt). I have used the same licence for the voice that I have created. There is information about recording your own diphones [here](http://festvox.org/bsv/x2401.html) and in the speech synthesis lecture by Professor Alan W Black [here](https://www.youtube.com/watch?v=eDjtEsOvouM&t=1459s).

My diphone speech synthesizer has a number of limitations. 

1. It uses a small dictionary of approximately 56,600 English words. If a word is not recognised by the dictionary it is skipped over. 

2. Apostrophes and other special characters are not used as inserting these into the database can cause errors. There are two types of apostrophes. Apostrophes of possession and contraction. Possessive apostrophes indicate ownership of something, like in the following sentence: "Dog's birthday". You have to enter the word name without an apostrophe e.g. dogs. Contraction apostrophes are used to shorten words. For example, "It's a nice day outside" which is expanded as "It is a nice day outside". Use the expanded version with the Talk Calendar diphone speech synthesizer. 

3. The pronunciation of some words is poor. There is very little information on-line about how to convert words to a diphone list. In many cases I have made an educated guess. More work is needed on word pronunciation.

Many open source and commercial speech synthesizers use the formant speech synthesis approach. For example, the excellent [eSpeak](https://espeak.sourceforge.net/) open source speech synthesizer for Linux and Windows uses a formant synthesis method. I had considered using eSpeak rather than code my own speech synthesizer but I discovered that some of its components may not be compatible with the GTK LGPL v2.1 license. For example, the IEEE80.c file [license](https://github.com/espeak-ng/espeak-ng/blob/c1d9341f86eee4b7a0da50712b627d8a76e92fea/src/libespeak-ng/ieee80.c) says "Copyright (C) 1989-1991 Apple Computer, Inc." which is very strange given that espeak has a GPL v3 [license](https://espeak.sourceforge.net/license.html). This is discussed further in the forum post [here](https://opensource.stackexchange.com/questions/11545/possibilities-to-use-a-gpl-v3-licensed-library-in-a-closed-source-game). 

Consequently, as Talk Calendar is a hobby project I would rather just use my own diphone-based speech synthesizer to avoid any potential license compatibility issues with using eSpeak. Even with its limitations, my diphone speech synthesizer is functional and has been coded from scratch using C and GTK4. It is more versatile than my previous speech synthesizer which was based on concatenating and playing back pre-recorded English words. 

I have also been working on a formant speech synthesizer the details of which can be found [here](https://github.com/crispinprojects/formant-synthesizer) but the sound quality is very robotic with buzzing and humming background noise.

## Versioning

[SemVer](http://semver.org/) is used for versioning. The version number has the form 0.0.0 representing major, minor and bug fix changes.

## Author

* **Alan Crispin** [Github](https://github.com/crispinprojects)

## Project Status

Active and under development.

## License

GTK is released under the terms of the [GNU Lesser General Public License version 2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html). Consequenty, Talk Calendar is licensed under the same LGPL v2.1 license.

## Acknowledgements

* [GTK](https://www.gtk.org/)

* GTK is a free and open-source project maintained by GNOME and an active community of contributors. GTK is released under the terms of the [GNU Lesser General Public License version 2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html).

* [GTK4 API](https://docs.gtk.org/gtk4/index.html)

* [GObject API](https://docs.gtk.org/gobject/index.html)

* [Glib API](https://docs.gtk.org/glib/index.html)

* [Gio API](https://docs.gtk.org/gio/index.html)

* [Geany](https://www.geany.org/) is a lightweight source-code editor (version 2 now uses GTK3). [GPL v2 license](https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt)

* [Sqlite](https://www.sqlite.org/index.html) is open source and in the [public domain](https://www.sqlite.org/copyright.html).

* [Diphone License](https://github.com/hypnaceae/DiphoneSynth/blob/master/diphones_license.txt)

* Diphone collection and synthesis Alan W. Black and Kevin Lenzo [2000](https://www.cs.cmu.edu/~awb/papers/ICSLP2000_diphone.pdf)

* [Fedora](https://fedoraproject.org/)

* [Ubuntu](https://ubuntu.com/download/desktop)

* [Debian](https://www.debian.org/)
