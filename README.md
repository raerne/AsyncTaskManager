# AsyncTaskManager
Simple thread-safe async task-queue.
## Install
```
mkdir .repo
cd .repo/
git clone https://github.com/Crascit/DownloadProject.git
```
In `test/CMakeLists.txt` change the following line to your `.repo` directory
```
set(REPO ~/CLionProjects/.repo)
```

## Build and test
```
 mkdir build
 cd build/
 cmake ..
 make -j4
 ./test/runUnitTests 
```