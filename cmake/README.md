This directory notes all QuadMat dependencies and how to satisfy them.

## Threading Building Blocks
Parallelism library from Intel. [TBB website](https://www.threadingbuildingblocks.org/).

Install TBB to your system using your favorite package manager or follow Intel's instructions.

 * macOS: `brew install tbb`
  
QuadMat uses [FindTBB](https://github.com/justusc/FindTBB) to find your system TBB library.

Alternate [FindTBB.cmake](https://github.com/Kitware/VTK/blob/master/CMake/FindTBB.cmake).
To bundle TBB with the library consider [wjakob's CMake-friendly TBB](https://github.com/wjakob/tbb).

