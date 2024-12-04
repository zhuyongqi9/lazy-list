# Lazylist: Your Terminal-Based File Management Solution  

*Imagine This Scenario，You’ve just joined a new company or research lab. One day, an IT veteran introduces you to something called **Linux**—an entirely different world of operating systems. They explain that you’ll need a magical tool called **SSH** to access it.*  

*After some effort, you successfully log in, only to discover that this system doesn’t have a graphical interface. Instead, everything must be done through something cryptically referred to as the **shell**.*  

*Now, you’re tasked with installing a piece of software. The expert casually tells you, "Just run a few commands: `wget`, then `cd`, and finally `xxx`. Easy!"*  

*You sit down at the terminal, sweating nervously as you try to follow their instructions. Then, in a moment of confusion, you accidentally type:`rm -rf *`*  

***
## What's Lazylist
**Lazylist** is an innovative GUI file management tool that runs seamlessly in your terminal. Inspired by macOS Finder, Lazylist eliminates the need to remember complicated Linux shell commands and protects you from the serious consequences of executing them incorrectly.  

## Key Features  
- **Essential File Operations**: Easily manage files and directories, extract compressed archives, and more.  
- **Favorites**: Quickly access your most-used files and directories.  
- **Trash Mechanism**: Safeguard against accidental deletions with a built-in trash system, so you can avoid the regret of an irreversible `rm -rf *`.  
- **User-Friendly Design**: Intuitive navigation makes file management simple and efficient, even for users unfamiliar with Linux commands.  

Lazylist is the ultimate tool for users who want the power of Linux without the steep learning curve of shell commands. Simplify your workflow and protect your files with **Lazylist**!  
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
