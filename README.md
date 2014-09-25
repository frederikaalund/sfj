# SFJ

SFJ is two things: The **black_label** support library and the **cave_demo** application. The latter showcases the former.

### black_label
 
[black_label](https://github.com/frederikaalund/sfj/tree/master/black_label/black_label) — Portable C++14/1z support libraries. Named after a whisky as with all of my support libraries.

 * [rendering](https://github.com/frederikaalund/sfj/tree/master/black_label/black_label/rendering) — Low-level: OpenGL wrappers. High-level: Multi-threaded [asset](https://github.com/frederikaalund/sfj/blob/master/black_label/black_label/rendering/assets.hpp) manager and a data-driven rendering [pipeline](https://github.com/frederikaalund/sfj/blob/master/black_label/black_label/rendering/pipeline.hpp).
 * [file_system_watcher](https://github.com/frederikaalund/sfj/blob/master/black_label/black_label/file_system_watcher.hpp) — Subscribe to file system events (file/directory creation/modification/deletion).
 * [utility](https://github.com/frederikaalund/sfj/tree/master/black_label/black_label/utility) — Mainly for convenience. [scoped_stream_suppression](https://github.com/frederikaalund/sfj/blob/master/black_label/black_label/utility/scoped_stream_suppression.hpp) for those annoying libraries that clutter `std::out`/`std::err`. A [checksum](https://github.com/frederikaalund/sfj/blob/master/black_label/black_label/utility/checksum.hpp) function when you just need a quick CRC32 checksum and nothing else.
 * [...And lots more!](https://github.com/frederikaalund/sfj/tree/master/black_label/black_label)

### cave_demo

[cave_demo](https://github.com/frederikaalund/sfj/tree/master/cave_demo) — A demo application to showcase the [black_label](https://github.com/frederikaalund/sfj/tree/master/black_label/black_label) libraries. Initially featured a cave setting (hence the name) but evolved from there.

 * Renders dynamic 3D scenes in real-time with physics.
 * The entire rendering pipeline is data-driven. I.e., a [rendering_pipeline.json](https://github.com/frederikaalund/sfj/blob/master/black_label/libraries/rendering/shaders/rendering_pipeline.json) file describes each rendering pass and the resource handling between passes.
 * Fast iteration times. The [asset](https://github.com/frederikaalund/sfj/blob/master/black_label/black_label/rendering/assets.hpp) manager and [file_system_watcher](https://github.com/frederikaalund/sfj/blob/master/black_label/black_label/file_system_watcher.hpp) libraries are combined to deliver file system-level synchronization of assets. E.g., edit a texture in Photoshop, save it, and the changes are reflected instantaneously in the real-time rendering. Likewise, edit a shader program, save it, and the new effect immediately appears. Same goes for the [rendering_pipeline.json](https://github.com/frederikaalund/sfj/blob/master/black_label/libraries/rendering/shaders/rendering_pipeline.json) file.
 * All aspects of the rendering are physically-based.
    * Area lights and the accompanying soft shadows.
    * Torrance-Sparrow and Oren-Nayar BRDFs.
    * Ambient occlusion (local approximation).
    * Global illumination based on photon differentials (work in progress).
    * ...Gamma correct, HDRI, LUT-based post processing, and everything you would expect from a state-of-the-art renderering pipeline.

### Deprecated Libraries

There are also many [deprecated libraries](https://github.com/frederikaalund/sfj/tree/master/deprecated/black_label/black_label) which I keep around for historical reasons. They are still good inspiration but have been superseeded by another (3rd party) library.

##### thread_pool

A good example is the thread_pool which I wrote before I embraced Intel's Threading Building Blocks. It was a good exercise on multi-threaded programming. I'm still really fond of the interface. See the [unit tests](https://github.com/frederikaalund/sfj/blob/master/deprecated/black_label/libraries/thread_pool/test/test_1.cpp) for an example. Basically, the `a | b` means *call `a` and `b` in parallel* and the `a >> b operator` means *call `a` and then call `b`*. Both `a` and `b` are *callable*s. I.e., anything that can be invoked like `a()`.

##### lsystem

Some libraries like the [lsystem](https://github.com/frederikaalund/sfj/blob/master/deprecated/black_label/black_label/lsystem.hpp) library I deprecated simply because I lost interest.

##### containers

There are also some [containers](https://github.com/frederikaalund/sfj/tree/master/deprecated/black_label/black_label/container) like the [darray](https://github.com/frederikaalund/sfj/blob/master/deprecated/black_label/black_label/container/darray.hpp) and the [svector](https://github.com/frederikaalund/sfj/blob/master/deprecated/black_label/black_label/container/svector.hpp). Both are very simply and use contiguous memory for storage. They have an STL-like interface complete with iterators (though no allocators). Without going into too much detail they make stricter usage assumptions than, say, `std::vector` which allows them to skip certain bound checks. I liked and used both the [darray](https://github.com/frederikaalund/sfj/blob/master/deprecated/black_label/black_label/container/darray.hpp) and the [svector](https://github.com/frederikaalund/sfj/blob/master/deprecated/black_label/black_label/container/svector.hpp) all over the place. They have been deprecated simply because it was too much of a hassle to maintain a fully conformant STL-like interface.

### Building

Your compiler must support subset of C++14/1z. The following compilers are known to work:
 * Visual Studio 14 CTP3.
 * Any recent release of Clang or GCC.

##### Dependencies 

Don't re-invent the wheel. E.g., don't implement a [thread_pool](https://github.com/frederikaalund/sfj/blob/master/deprecated/black_label/libraries/thread_pool/test/test_1.cpp). Therefore, black_label rely on *a lot* of 3rd party libraries:

 * [Common dependencies](https://github.com/frederikaalund/sfj/blob/master/black_label/cmake/dependencies_common.cmake)
   * Boost
   * GLM
   * TBB
 * [Rendering dependencies](https://github.com/frederikaalund/sfj/blob/master/black_label/cmake/dependencies_rendering.cmake)
   * GLEW
   * OpenGL
   * SFML (graphics module)
     * Winmm (on Windows)
     * JPEG
     * Freetype
   * ASSIMP
     * ZLIB
   * FBX (optional)
   * OpenColorIO
     * TinyXML
     * YAMLCPP
   * GLI
   * Squish

Furthermore, the cave_demo [depends on](https://github.com/frederikaalund/sfj/blob/master/cave_demo/CMakeLists.txt) (besides black_label):

 * Boost
   * program_options
   * log
   * log_setup
 * SFML
   * system
   * window
   * CoreFoundation, Cocoa, IOKit, and Carbon (on OS X)
   * VTune (optional)
