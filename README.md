# mediakit
mediakit is a media converter built on top of ffmpeg. it is meant to trim videos, transcode them, transform media (such as audio/video/image) and things of that nature.

the current scope is to be a damn good video trimmer before we get to generic media content. this is because there are no good video trimmers


## build
requires Qt6 widgets and multimedia libraries

build is just regular cmake workflow:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

install
```sh
cmake --install build --prefix dist
```


## roadmap
- basics
  - [x] video playback/seeking via QMediaPlayer
  - [x] basic trim markers
  - [x] basic video trimming
  - [x] basic job queue/system, calling and parsing ffmpeg outputs
  - [x] better trim timeline
    - [ ] zoom in/out on the timeline for big videos
  - [x] media list dock, imports and drag and drop
  - [x] properties dock, likely a tree or something of options to change for selected media
  - [x] bundle ffmpeg some way
- properties
  - [x] more than just trimming
  - [x] get info about media
  - [x] choose target output containers
  - [x] choose target codecs
  - [x] choose target bitrate etc...
  - [ ] other tools like extract video to audio, common things like this

and more things i haven't thought about at the time of writing such as CI, error handling, settings, about...


## license
TBD, likely GPLv3.
