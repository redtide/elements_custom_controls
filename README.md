# elements_custom_controls

Custom controls made with [Elements][1] GUI library.

In order to build these projects, you need to first `git clone` the Elements
repository and use the `artist_port` branch.
Please refer also to [Elements setup guide][2].

In a terminal, from one of these projects directory:

```sh
mkdir build && cd build
cmake -DELEMENTS_ROOT="/your/path/to/elements" ..
cmake --build .
```

## Curve Editor

A 2D time curve editor, allowing user to write and display the following curves:

- Linear
- Log Exp
- Cubic Bezier
- Quadratic Bezier
- Cubic Spline

## Oscilloscope

A 2D XY Oscilloscope in development.


[1]: https://github.com/cycfi/elements
[2]: https://cycfi.github.io/elements/setup
