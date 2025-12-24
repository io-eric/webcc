# Audio API

The `webcc::audio` module provides a simple interface for playing audio files.

## Header

```cpp
#include "webcc/audio.h"
```

## Functions

### `create_audio`

Creates a new audio object from a source URL.

```cpp
webcc::handle create_audio(webcc::string_view src);
```

### Playback Control

```cpp
void play(webcc::handle handle);
void pause(webcc::handle handle);
```

### Properties

```cpp
void set_volume(webcc::handle handle, float vol);
void set_loop(webcc::handle handle, uint8_t loop);
float get_current_time(webcc::handle handle);
float get_duration(webcc::handle handle);
```
