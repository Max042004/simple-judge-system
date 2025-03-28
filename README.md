# c-starter-kit
A lightweight C development boilerplate with Python scripting integration, linenoise input, and Makefile build system. Quick-start your CLI C projects with modern tooling.

## prerequisite
### Make
windows should install GNU make first, and add in system variable, and reboot.
(if windows user want to use valgrind should clone the project to WSL or using Docker)  
### valgrind
Linux: 
```
sudo apt update
sudo apt install valgrind
```
Macos:
```
brew install valgrind
```
windows:
you should clone the project to WSL.

Or using docker:
1. install docker desktop for windows
2. open docker desktop
3. `cd c-start-kit`
4. create Dockerfile
5. write below code into Dockerfile
   ```
   FROM ubuntu:latest
   
   RUN apt update && apt install -y g++ valgrind make python3

   WORKDIR /project
   
   COPY . .
   ```
7. run `docker run -it ubuntu`
8. run `docker build -t valgrind-env .`
9. run `docker run --rm -it valgrind-env`
