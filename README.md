# lowel

![pre-release version badge](https://img.shields.io/github/v/release/c-krit/lowel?include_prereleases)
![repo-size badge](https://img.shields.io/github/repo-size/c-krit/lowel)
![license badge](https://img.shields.io/github/license/c-krit/lowel)

A library that allows you to load and save tiled or non-tiled 2D maps for raylib projects.

**WARNING: This library is in an early alpha stage, use it at your own risk.**

## Features

- Load and unload tiled or non-tiled 2D map data from a `.json` file or from memory
- Draw the entire or part of a map based on the current player positions

## Building

After building and installing [raylib](https://github.com/raysan5/raylib), run the following command to obtain the static version of lowel:

```console
$ make && cd lowel/lib
```

... or you can copy-paste everything inside `lowel/include` and `lowel/src` to your project directory and compile your project with these files.

## Examples

![example: bermuda](https://raw.githubusercontent.com/c-krit/lowel/main/examples/bermuda/bermuda.png)

The source code for all examples can be found in the `examples` directory.

## License

MIT License