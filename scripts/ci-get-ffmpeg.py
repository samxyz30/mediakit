import platform
import urllib.request
import tarfile
import zipfile
from pathlib import Path

DISTRIB_URL = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest"
ARCHIVE_NAME = "ffmpeg-n9.0-latest-{os}-gpl-shared-9.0.{ext}"

def main():
  if platform.system() == "Windows":
    is_os = "win64"
    is_ext = "zip"
  elif platform.system() == "Linux":
    is_os = "linux64"
    is_ext = "tar.xz"


  output = Path("./dist/bin/tools/")
  output.mkdir(parents=True, exist_ok=True)

  archive_real = ARCHIVE_NAME.format(os=is_os, ext=is_ext)
  url = f"{DISTRIB_URL}/{archive_real}"
  temp = Path(f"temparchive.{is_ext}")

  print(f"downloading ffmpeg from: {url}")
  urllib.request.urlretrieve(url, temp)

  if is_ext == "zip":
    with zipfile.ZipFile(temp) as zip:
      members = [n for n in zip.namelist() if "/bin/" in n and not n.endswith("/")]
      for memb in members:
        target = output / Path(memb).name
        target.write_bytes(zip.read(memb))

  else:
    with tarfile.open(temp, "r:*") as tar:
      members = [n for n in tar.getmembers() if n.isfile() and "/bin/" in n.name or "/lib/" in n.name]
      for memb in members:
        src = tar.extractfile(memb)
        if src is not None:
          target = output / Path(memb.name).name
          target.write_bytes(src.read())
          target.chmod(0o755)

  temp.unlink()


if __name__ == "__main__":
  main()
