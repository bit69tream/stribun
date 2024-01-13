## STRIBUN
A space shooter inspired by games like Enter The Gungeon, Furi and Nuclear Throne. _(And ofc Asteroids ;>)_

![$(STRIBUN)](logo.png "$(STRIBUN)")

## Building from source
**You don't need Raylib installed on your system to compile STRIBUN!**
**CMake will download everything automatically!**

### Desktop
```sh
$ cmake -B build
$ cmake --build build
```

### Web
```sh
$ mkdir build-web
$ cd build-web
$ emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release
$ emmake make
```

## Controls

 - <kbd>W</kbd>/<kbd>A</kbd>/<kbd>S</kbd>/<kbd>D</kbd> or <kbd>E</kbd>/<kbd>S</kbd>/<kbd>D</kbd>/<kbd>F</kbd> - movement
 - <kbd>Left mouse button</kbd> - shoot
 - <kbd>Right mouse button</kbd> - dash

## Screenshots

![uziel battle](screenshots/uziel.gif)
![rigor battle](screenshots/rigor.gif)

## Developers

 - bit69tream - Everything 😎

## Links
 - itch.io Release: https://bit69tream.itch.io/stribun

## Used Resources

### Sound Effects
 - [Shapeforms Audio Free Sound Effects](https://shapeforms.itch.io/shapeforms-audio-free-sfx)

### Music
 - Main menu - [Drozerix - Stardust Jam](https://modarchive.org/module.php?201039) Public Domain
 - Uziel Enkidas - [once is not enough](https://modarchive.org/module.php?170002) [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)
 - Rigor Mortis - [Red Dream](https://modarchive.org/module.php?170064) Public Domain

## License

This game sources are licensed under an unmodified zlib/libpng license, which is an OSI-certified, BSD-like license that allows static linking with closed source software. Check [LICENSE](LICENSE) for further details.

*Copyright (c) 2024 bit69tream*
