# Third-Party Notices

This project uses the following third-party components. Their license terms are
listed below and must be preserved in any redistribution.

---

## sfxr (synthesis algorithm)

The synthesis engine in `Source/SfxrEngine/` is a port of
[sfxr](http://www.drpetter.se/project_sfxr.html) by Tomas Pettersson (DrPetter),
originally distributed as `sfxr-sdl-1.2.1`. The original source is kept under
`reference/` for reference.

License: MIT

```
Copyright (c) 2007 Tomas Pettersson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
```

---

## JUCE

This project builds with [JUCE](https://juce.com/) (version 8.0.15, obtained via
CMake `FetchContent`), used here under the AGPLv3. JUCE is copyright © Raw
Material Software Limited.

---

## VST3 SDK

The VST3 SDK is bundled with JUCE's audio plugin client and is provided by
Steinberg Media Technologies GmbH under the Steinberg VST 3 SDK license.
