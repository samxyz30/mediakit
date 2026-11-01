# mediakit usage reference

if you want a quick reference for this software, this is it.

mediakit is a converter that aims to be good at trimming and transcoding/converting content, primarily audio/video.

as of writing it's still a work in progress, help whether via issue reporting or pull requests is heavily appreciated.


## interface
on the top you have your toolbar with common actions, such as:
- `open media`: opens and adds file(s) to the media list. you can also drag and drop files directly on the media list docker.

- `trim`: trims the video **losslessly**, it does this by simply copying and trimming the video streams.
  > NOTE: this method is fast, however can cause weird sync glitches and issues in certain encodings. **if you're unsure, use `transcode` instead.**

- `transcode`: transcodes the video using the selected preset and output options you picked in the properties docker.

- `settings`: opens the settings dialog.


---

there's 4 primary dockers:
- `media list`: your imported media. you can drag and drop files here and double click to select one.

- `properties`: properties for the selected media, shows information about it such as duration, size, streams... and has a preset dropdown and output configuration.

- `queue`: the job queue, whenever you trim or transcode something it gets put here with a log of what it's doing and options to manage the selected job.

- the media player/trimmer component, which is part of the window. this is where you primarily do your trimming



## typical workflow
say you want to trim something. you would simply:

1. import the video/audio file
2. select your trim range until you're happy **(shortcuts: `I` to mark start, `O` to mark end)**
3. if the trim range seems small, use the mouse wheel to zoom in/out.
4. click `trim`/`transcode`

if you want to compress or encode something:

1. import the video/audio file
2. under properties, if you're in a rush, pick a preset from the dropdown
  (or if not, configure it under the output tree).
3. click `transcode`

