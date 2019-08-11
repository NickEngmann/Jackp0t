# Jackp0t
Modifies your DEFCON27 Badge into a Jackp0t badge to force regular attendees into a rickroll mode

--- TODO::
Contributors:
- [](Josiah's Twitter)
- [](Nick's Twitter)
- [](GP's Twitter)


## Table of Contents
--- TODO::

### TLDR/ABSTRACT/The Challenge Was:
--- TODO::

## Instructions
Go ahead and do the damn thing. Clone this repo, take the binary and load it into MCUXpresso
--- TODO::

### Development Environment
Windows Host with USB 2.0 Ports for MCUXpresso OR Windows/OSX/Linux for GDP Black Magic Probe

#### Hardware

- DEFCON27 Badge
- [Micro 1v8 USB Serial UART](http://jim.sh/1v8/)
- [Tag-Connect TC2050-IDC-NL-050-ALL (normal orientation)](http://www.tag-connect.com/node/199)
- trim alignment pins if using w/ mounted gemstone • vtref provides 1.8V i/o level to debug probe
- [NXP LPC-Link2](https://www.nxp.com/design/microcontrollers-developer-resources/lpc-microcontroller-utilities/lpc-link2:OM13054)
OR A:
- [Black Magic Probe](https://1bitsquared.com/products/black-magic-probe)

#### Software

- [nxp MCUXpresso IDE 10.2.1](https://media.defcon.org/DEF%20CON%2027/DEF%20CON%2027%20badge/Development%20Environment/MCUXpressoIDE_10.2.1_795.exe)
- [FRDM-KL27Z SDK 2.4.1](https://media.defcon.org/DEF%20CON%2027/DEF%20CON%2027%20badge/Development%20Environment/SDK_2.4.1_FRDM-KL27Z.zip)
- [GDP]()
--- TODO:: Link to GDP

### Setting up the Hardware

#### LPC-Link2

#### Black Magic Probe
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
- [Ross Last Name here](Link to Ross's Personal Website Blog)
- [Name of the Guy WHo Designed the Board]()

### Privacy Policy
TODO:::

### Copyright Policy
TODO:::
