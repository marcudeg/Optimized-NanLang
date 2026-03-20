# NanLang Graphics Toolkit (`nan_graphics.nl`)

`nan_graphics.nl` is a declarative NanLang UI prelude for the current native emitter.
It is intentionally linear: instead of functions and event loops, the source emits
top-level `ui` directives that the compiler can turn into a GTK launcher.

## Usage

Concatenate the prelude and a scene file:

```bash
cat lib/nan_graphics.nl examples/gui_test_interface.nl > out/gui_demo.nl
./bin/nanlang compile out/gui_demo.nl --output out/gui_demo.elf
./bin/nanlang run out/gui_demo.elf
```

The same scene can be rendered directly:

```bash
./bin/nanlang ui-render out/gui_demo.nl
```

## Supported Directives

- `ui app begin;`
- `ui app end;`
- `ui window create <id> <width> <height>;`
- `ui window title <id> "<title>";`
- `ui layout vertical <window_id> <spacing>;`
- `ui layout horizontal <window_id> <spacing>;`
- `ui label <window_id> <widget_id> "<text>";`
- `ui button <window_id> <widget_id> "<text>";`
- `ui slider <window_id> <widget_id> <min> <max> <current>;`
- `ui chart begin <window_id> <chart_id> <samples>;`
- `ui chart point <chart_id> <index> <value>;`
- `ui chart commit <chart_id>;`
- `ui frame present <window_id>;`

## Bridge Model

The compiler keeps the non-UI execution path intact, but when it sees `ui`
directives it emits a self-contained launcher that calls the native GTK renderer.
That keeps the toolchain realistic while still giving you an actual window.

## Files

- `lib/nan_graphics.nl`
- `examples/gui_test_interface.nl`
