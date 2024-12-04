## Install


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