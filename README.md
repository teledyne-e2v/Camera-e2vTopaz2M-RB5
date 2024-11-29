# Camera-e2vTopaz2M-RB5
Camera driver for Qualcomm RB5

Ubuntu 20 system is recommended

## Install and Run sdkmanager
Install docker

    sudo apt  install docker.io

get the sdk manager:
```
sudo apt-get install wget
```

```
sudo wget https://sdkgit.thundercomm.com/api/v4/projects/4649/repository/files/turbox-sdkmanager-setup.sh/raw?ref=main -O /usr/bin/turbox-sdkmanager-setup.sh
```
make it executable:
```
sudo chmod +x /usr/bin/turbox-sdkmanager-setup.sh
```

## Start SDK manager and initialize the project

Open SDK-Manager with OS-version 20.04 via the following command
```
/usr/bin/turbox-sdkmanager-setup.sh --os-version 20.04
```
Setup your git login
```
git config --global user.name "romain.guiguet@teledyne.com"
git config --global user.email "romain.guiguet@teledyne.com"
```
Create the ssh Key (inside the docker)
```
ssh-keygen -t rsa -C "romain.guiguet@teledyne.com"
```
Copy tkey the ssh key to your Thundercomm/Thundersoft contact to get access to the repo
```
cat ~/.ssh/id_rsa.pub
```
Then init the repo and sync
```
repo init -u ssh://romain.guiguet@teledyne.com@partner.thundercomm.com:9418/manifest -b SDK.Frontrunner-rb5165-lu2.0-dev -m Frontrunner-rb5165-lu2.0-dev.xml -g all --repo-url=ssh://romain.guiguet@teledyne.com@partner.thundercomm.com:9418/tools/repo --repo-branch=stable --no-repo-verify
```

```
repo sync -cd --no-tags -j4
```

## Copy the patches and apply them
Open two terminals:
- [DOCKER]: terminal where the sdk manager has been started in the previous step
- [DRIVER]: treminal where this repository has been clone, containing the divers files and patches
Please follow the next steps in the correct order to apply properly the different patches

That is also require to note the CONTAINER ID that will be used in next steps to copy the patche files to docker:
[DRIVER]:
```
sudo docker ps
```
terminal render:
```
CONTAINER ID   IMAGE                                                                COMMAND       CREATED       STATUS       PORTS     NAMES
8d9de487cb01   public.ecr.aws/k5o4b3u5/thundercomm/turbox-sdkmanager-20.04:v1.2.2   "/bin/bash"   5 hours ago   Up 5 hours             turbox-sdkmanager-20.04_v1.2.2_1000
```
The container ID here is 8d9de487cb01, please replace it in all the next commands by your own ID.

### Recipe correction
This correction has to be applied manually by copying the following files:

[DRIVER]:
```
sudo docker cp recipe/python3-ubuntu.bb 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/poky/meta-qti-ubuntu/recipes-toolchain/ubuntu/.
sudo docker cp recipe/ptool-native_git.bb 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/poky/meta-qti-bsp/recipes-devtools/ptool/.
```

### Base driver :
[DRIVER]:
```
sudo docker cp Patch 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/src/vendor/qcom/proprietary/.
```
[DOCKER]:
```
cd  /home/turbox/lu.um.3.3.1/apps_proc/src/vendor/qcom/proprietary/
git apply Patch/0001-Camera-e2vTopaz2M-bring-up.patch --whitespace=nowarn
```
### Control driver :
[DRIVER]:
```
sudo docker cp control/0001-Camera_Teledyne2-Add-device-node-e2vTopaz.patch 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/src/kernel/msm-5.4/techpack/camera/.
sudo docker cp control/0002-Camera_Teledyne2-Skip-sof-freeze.patch 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/src/kernel/msm-5.4/techpack/camera/.
sudo docker cp control/0003-Camera_Teledyne2-Add-read-func-for-e2vTopaz.patch 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/src/kernel/msm-5.4/techpack/camera/.
```
[DOCKER]:
```
cd  /home/turbox/lu.um.3.3.1/apps_proc/src/kernel/msm-5.4/techpack/camera
git apply 0001-Camera_Teledyne2-Add-device-node-e2vTopaz.patch --whitespace=nowarn
git apply 0002-Camera_Teledyne2-Skip-sof-freeze.patch --whitespace=nowarn
git apply 0003-Camera_Teledyne2-Add-read-func-for-e2vTopaz.patch --whitespace=nowarn
```
### GST example :
[DRIVER]:
```
sudo docker cp gst/0001-GST-add-teledyne-example.patch 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/src/vendor/qcom/opensource/gst-plugins-qti-oss/.
```
[DOCKER]:
```
cd  /home/turbox/lu.um.3.3.1/apps_proc/src/vendor/qcom/opensource/gst-plugins-qti-oss
git apply 0001-GST-add-teledyne-example.patch --whitespace=nowarn
```
### Library update
This correction has to be applied manually by copying the following file:
[DRIVER]:
```
sudo docker cp bin/camx_0.1_aarch64.tar.gz 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/prebuilt_HY11/.
```

## Build the image 
In the docker:
```
cd /home/turbox
```
Start the build:
```
./turbox_build.sh -alv debug
```
Note that is you have a recipe error like ```python3-ubuntu.bb``` you have to check the version following the URL.
If you need to modify the version, the checksum will vary also: please run the build agin to get the new value.
After any change in the recipe, to take it into account that is require to remove the following file before running the build script:
```
cd lu.um.3.3.1/apps_proc/
rm -rf build-qti-distro-ubuntu-fullstack-debug
```


Create the image:
```
cd /home/turbox
./turbox_build.sh -ul
```
```
./turbox_build.sh --zip_flat_build -l
```
Flat image is then found  in ```turbox/output``` folder
You’ll see 2 variants: “RB5” and “RB5N”. The correct one to choose for your project is “RB5” (without ‘N’)

```
sudo docker cp 8d9de487cb01:/home/turbox/turbox/output/FlatBuild_Turbox-RB5_xx.xx_lu2.0.l.userdebug.20240923.191846_RB5.zip .
```

## Flash the image
Copy the ZIP file in Windows PC and extract it.


Enter 9008 Emergency Download (EDL) mode with F_DL key.
Follow these steps to force the device to enter 9008 (EDL) mode.
1. Press the F_DL key and hold.
2. Connect the device to the host machine via USB Type-C cable, wait 2 seconds and release the F_DL key.
3. Connect the device to a 12 V Power supply.

adb command enter 9008 (EDL) mode.
```
adb root
adb reboot edl
```

Open QFIL (https://qfiltool.com/) and follow these steps
1. SelectPort: Ensure that the 9008 port is found.
2. Select Flat build, click Browser… to select programmer in the image folder: ```prog_firehose_ddr.elf```
3. Click Load XML:

    a. select the following rawprogramxx.xml and click Open
![image](https://github.com/user-attachments/assets/0c197e88-2b65-41a2-8e3b-0d1646a5b0f1)

     b. select all patchxx.xml and click Open
![image](https://github.com/user-attachments/assets/721adab8-bb8c-4430-94db-c1f9f272705d)

6. Select ***ufs*** device type configuration.
7. Click on Dowload

## Test the driver

### Preview over TCP forwarding

From the PC connecet on USB-C to the platform:

Terminal window 1:
```
adb root && adb remount && adb shell mount -o remount,rw /
adb shell
```
start video the pipeline
```
gst-launch-1.0 -e qtiqmmfsrc camera=0 ! video/x-raw\(memory:GBM\),format=NV12,width=1920,height=1080,framerate=60/1 ! qtic2venc ! h264parse ! queue ! h264parse config-interval=-1 ! mpegtsmux name=muxer ! queue ! tcpserversink port=8900 host=127.0.0.1
```

Terminal window 2:
```
adb forward tcp:8900 tcp:8900
vlc -vvv tcp://127.0.0.1:8900
```

### Preview from the platform connected to HDMI screen

```
export XDG_RUNTIME_DIR=/run/user/root && gst-launch-1.0 qtiqmmfsrc camera=0 !video/x-raw\(memory:GBM\), format=NV12,width=1920,height=1080, framerate=60/1 ! waylandsink fullscreen=true async=true sync=false
```

### Log creation for debug

In case of facing issue, that is possible to generate a log file.

[DRIVER]:
```
adb root
adb remount
```
Start collect logs:
```
adb logcat -b all > logfile.txt
```
Then start reproducing the problem，after reproducing the problem, use Ctrl+C to stop log scraping and provide the log file on this case
