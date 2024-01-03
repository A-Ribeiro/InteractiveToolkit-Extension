> For this is how God loved the world:  
he gave his only Son, so that everyone  
who believes in him may not perish  
but may have eternal life.  
  \
John 3:16

This is an extension of the [InteractiveToolkit](https://github.com/A-Ribeiro/InteractiveToolkit).

Here you can deal with image loading (PNG/JPG), Binary data writing and reading, Pixel Font Glyph management, and 3D model storing.

## Dependency

The main dependency is the [https://github.com/A-Ribeiro/InteractiveToolkit](https://github.com/A-Ribeiro/InteractiveToolkit) itself.

You can take a look at the example repository at: [https://github.com/A-Ribeiro/InteractiveToolkit-Examples](https://github.com/A-Ribeiro/InteractiveToolkit-Examples).

## How to Import the Library?

__HTTPS__

```bash
git init
mkdir libs
git submodule add https://github.com/A-Ribeiro/InteractiveToolkit.git libs/InteractiveToolkit
git submodule add https://github.com/A-Ribeiro/InteractiveToolkit-Extension.git libs/InteractiveToolkit-Extension
```

In your CMakeLists.txt:

```cmake
# at the beginning of yout root 'CMakeLists.txt':

unset (CMAKE_MODULE_PATH CACHE)
unset (CMAKE_PREFIX_PATH CACHE)

# include libs in sequence:

add_subdirectory(libs/InteractiveToolkit "${CMAKE_BINARY_DIR}/lib/InteractiveToolkit")
add_subdirectory(libs/InteractiveToolkit-Extension "${CMAKE_BINARY_DIR}/lib/InteractiveToolkit-Extension")

# in your lib or binary file:

add_library(YOUR_LIBRARY STATIC)
target_link_libraries(YOUR_LIBRARY PUBLIC InteractiveToolkit-Extension)
```

You can update your submodules with the command:

```bash
git submodule update --remote --merge
```

__SSH__

```bash
git init
mkdir libs
git submodule add git@github.com:A-Ribeiro/InteractiveToolkit.git libs/InteractiveToolkit
git submodule add git@github.com:A-Ribeiro/InteractiveToolkit-Extension.git libs/InteractiveToolkit-Extension
```

In your CMakeLists.txt:

```cmake
# at the beginning of yout root 'CMakeLists.txt':

unset (CMAKE_MODULE_PATH CACHE)
unset (CMAKE_PREFIX_PATH CACHE)

# include libs in sequence:

add_subdirectory(libs/InteractiveToolkit "${CMAKE_BINARY_DIR}/lib/InteractiveToolkit")
add_subdirectory(libs/InteractiveToolkit-Extension "${CMAKE_BINARY_DIR}/lib/InteractiveToolkit-Extension")

# in your lib or binary file:

add_library(YOUR_LIBRARY STATIC)
target_link_libraries(YOUR_LIBRARY PUBLIC InteractiveToolkit-Extension)
```

You can update your submodules with the command:

```bash
git submodule update --remote --merge
```

## Authors

***Alessandro Ribeiro da Silva*** obtained his Bachelor's degree in Computer Science from Pontifical Catholic 
University of Minas Gerais and a Master's degree in Computer Science from the Federal University of Minas Gerais, 
in 2005 and 2008 respectively. He taught at PUC and UFMG as a substitute/assistant professor in the courses 
of Digital Arts, Computer Science, Computer Engineering and Digital Games. He have work experience with interactive
software. He worked with OpenGL, post-processing, out-of-core rendering, Unity3D and game consoles. Today 
he work with freelance projects related to Computer Graphics, Virtual Reality, Augmented Reality, WebGL, web server 
and mobile apps (andoid/iOS).

More information on: https://alessandroribeiro.thegeneralsolution.com/en/
