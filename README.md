# Jackp0t
Modifies your DEFCON27 Badge into a Jackp0t badge to force regular attendees into a rickroll mode

-- TODO: Video

Contributors:
- [Halcy0nic](https://twitter.com/Halcy0nic)
- [mlaertean](https://twitter.com/mlaertean)
- [Nick Engmann](https://twitter.com/NickySlicksHaha)

# Table of Contents
1. [The Challenge](#the-challenge)
2. [Required Hardware](#required-hardware)
3. [Required Software](#required-software)
4. [Flashing](#flashing)
    1. [LPC-Link2 Setup](#lpc-link2-setup)
    2. [Black Magic Probe Setup](#black-magic-probe-setup)
5. [Future Ideas & Uses]()
6. [Credits](#credits)
7. [Copyright](#copyright)

## The Challenge
--- TODO::


## Required Hardware

- [DEFCON27 Badge](https://hackaday.com/2019/08/08/first-look-at-def-con-27-official-badge-kingpin-is-back/)
- [Tag-Connect TC2050-IDC-NL-050-ALL (normal orientation)](http://www.tag-connect.com/node/199)
- trim alignment pins if using w/ mounted gemstone • vtref provides 1.8V i/o level to debug probe
- [NXP LPC-Link2](https://www.nxp.com/design/microcontrollers-developer-resources/lpc-microcontroller-utilities/lpc-link2:OM13054) OR A [Black Magic Probe](https://1bitsquared.com/products/black-magic-probe)
- Optional - [Micro 1v8 USB Serial UART](http://jim.sh/1v8/) OR [Black Magic Probe](https://1bitsquared.com/products/black-magic-probe)

## Required Software

Windows Host with USB 2.0 Ports for MCUXpresso OR Windows/OSX/Linux for GDP Black Magic Probe

- [nxp MCUXpresso IDE 10.2.1](https://media.defcon.org/DEF%20CON%2027/DEF%20CON%2027%20badge/Development%20Environment/MCUXpressoIDE_10.2.1_795.exe)
- [FRDM-KL27Z SDK 2.4.1](https://media.defcon.org/DEF%20CON%2027/DEF%20CON%2027%20badge/Development%20Environment/SDK_2.4.1_FRDM-KL27Z.zip)
- [GDP]()
--- TODO:: Link to GDP

## Flashing
Clone this repo, take the binary and load it into MCUXpresso

### LPC-Link2 Setup

### Black Magic Probe Setup
    $ (gdb) target extended-remote /dev/cu.usbmodem <some ##>

    $ (gdb) monitor swdp scan

Target voltage: 1.8V
Available Targets:
No. Att Driver
 1      ARM Cortex-M
 2      Kinetis Recovery (MDM-AP)

    $ (gdb) attach 1

Attaching to Remote target
warning: while parsing target memory map (at line 1): Required element <memory> is missing
0x00003f52 in ?? ()
    $ (gdb) set mem inaccessible-by-default off

    $ (gdb) file dc27_badge-8-9-2019-0.axf || human.bin || dc27_badge.axf || dc27_badge.hex

A program is being debugged already.
Are you sure you want to change the file? (y or n) y
Reading symbols from dc27_badge-8-9-2019-0.axf...done.

    $ (gdb) load

Loading section .text, size 0xc954 lma 0x0
Load failed

# Credits
- [DEFCON Badge Hacking Thread - Reddit](https://www.reddit.com/r/Defcon/comments/cnn2x7/dc_27_badge_hacking_thread/)
- [Ross Schlaikjer](https://rhye.org/)
- [The Kingpin, Joe Grand](https://twitter.com/joegrand)

# License

This project is [MIT licensed](./LICENSE.md).
