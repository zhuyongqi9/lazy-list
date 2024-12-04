## Install
Currently only macos and ubuntu provided compiled binary

https://github.com/zhuyongqi9/lazylist/releases/tag/1.1.1

## How to build

### Mac OS

**Install dependency libraries**
```
brew install cmake spdlog
```

**build**
```
mkdir build
cd build
cmake --build . --target lazylist -j 16
```


### Ubuntu
**Install dependency libraries**
```
sudo apt install cmake libspdlog-dev
```

**build**
```
mkdir build
cd build
cmake --build . --target lazylist -j 16
```
