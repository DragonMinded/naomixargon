# Xargon for Sega Naomi

This is a port of the released source code for Xargon Episodes 1-3 for the Sega Naomi platform. If you want
to compile it, you will need to first set up the libnaomi toolchain at https://github.com/DragonMinded/libnaomi.
The original game files were also released as freeware so they are included in the release of Xargon for
Sega Naomi! Much thanks to Allen Pilgrim for releasing all of this and allowing me to play one of my favorite
games as a kid on a whole new platform!

Note that this is neither designed for, nor will ever be coded to take coin drops. This is a hobby port and
I don't want people making money running this in public based on my hard work. Somebody else may do the work
to make this happen, but I certainly won't be that person!

## Releases

The full version of the game is available in the releases directory! Load it in your favorite editor or net boot it.

## Default Controls

* The 1P joystick controls standard player movement as well as menu navigation.
* 1P start is used to, select menu entries as well as pull up in-game inventory.
* 2P start is used to back out of menus.
* 1P button 1 is fire/throw rocks/etc.
* 1P button 2 is jump.

## Known Issues

* Episode select has no select or cursor movement sound.
* Title screen corruption on fade-in of main menu, fade-in and fade-out don't seem to work.
* Screen dimming/undimming is not currently implemented.
* Somewhat laggy gameplay and control inputs, needs optimization or engine fixes.
* No test menu, game options, or control remapping.
* No game saving or loading.
* Cannot enter name into high scores, high scores do not save.

## Acknowledgements

The original source code was uploaded to github here: https://github.com/dos-games/vanilla-xargon
Malvineous has a SDL port that was helpful as a reference for porting this: https://github.com/Malvineous/xargon
Of course, thanks to Allen Pilgrim for coding the game as well as releasing the source and assets!
