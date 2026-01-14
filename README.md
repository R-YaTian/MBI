# MBI
MBI or Mohsaua-Buoh-Installer is a No-Bullshit NSP, NSZ, XCI, and XCZ Installer for Nintendo Switch

![Preview](doc/preview.jpg)

## Features
- Installs NSP/NSZ/XCI/XCZ files from your SD card
- Installs NSP/NSZ/XCI/XCZ files from your USB HDD exFAT, NTFS and EXT2/3/4 (NTFS and EXT2/3/4 currently works only with AMS)
- Installs NSP/NSZ/XCI/XCZ files and split NSP/XCI files over LAN or USB from tools such as [NS-USBloader](https://github.com/developersu/ns-usbloader)
- Installs NSP/NSZ/XCI/XCZ files over HTTP Directory Indexing like Nginx or Apache
- Verifies NCAs by header signature before they're installed

## Coloring support
To use this, you must place the "color.json" at "sdmc:/config/MBI" dir. See [example](doc/color.json) for more details.  
Use your wanted HEX colors -> https://www.w3schools.com/colors/colors_picker.asp

## Thanks to
- dezem and other contributors for original [AtmoXL-Titel-Installer](https://github.com/dezem/AtmoXL-Titel-Installer)
- Huntereb for [Awoo-Installer](https://github.com/Huntereb/Awoo-Installer)
- Adubbz and other contributors for [Tinfoil](https://github.com/matt-teix/Tinfoil)
- XorTroll for [Plutonium](https://github.com/XorTroll/Plutonium) and [Goldleaf](https://github.com/XorTroll/Goldleaf) and [libnx-ext](https://github.com/XorTroll/libnx-ext)
- nicoboss for [NSZ/XCZ](https://github.com/nicoboss/nsz) support
- DarkMatterCore, XorTroll and Rhys Koedijk for [libusbhsfs](https://github.com/DarkMatterCore/libusbhsfs)
