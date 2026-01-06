# Audio API

The `webcc::audio` module provides a simple interface for playing audio files.

## Header

```cpp
#include "webcc/audio.h"
```

## Functions

### `create_audio`

Creates a new audio object from a source URL. Returns an `Audio` handle, which can be implicitly converted to `DOMElement` for use with DOM functions.

```cpp
webcc::Audio create_audio(webcc::string_view src);
```

### Playback Control

```cpp
void play(webcc::Audio handle);
void pause(webcc::Audio handle);
```

### Properties

```cpp
void set_volume(webcc::Audio handle, float vol);
void set_loop(webcc::Audio handle, uint8_t loop);
float get_current_time(webcc::Audio handle);
float get_duration(webcc::Audio handle);
```
