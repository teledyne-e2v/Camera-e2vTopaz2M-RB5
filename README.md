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

## Copy the patch and apply it

### Copy Patch to docker (from outside the docker)
Find the docker id:
```
sudo docker ps
```

terminal render:
```
CONTAINER ID   IMAGE                                                                COMMAND       CREATED       STATUS       PORTS     NAMES
8d9de487cb01   public.ecr.aws/k5o4b3u5/thundercomm/turbox-sdkmanager-20.04:v1.2.2   "/bin/bash"   5 hours ago   Up 5 hours             turbox-sdkmanager-20.04_v1.2.2_1000
```
copy the patch file using docker ID:
```
sudo docker cp Patch 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/src/vendor/qcom/proprietary/.
```
Copy recipe corrections:
```
sudo docker cp python3-ubuntu.bb 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/poky/meta-qti-ubuntu/recipes-toolchain/ubuntu/.
```
```
sudo docker cp ptool-native_git.bb 8d9de487cb01:/home/turbox/lu.um.3.3.1/apps_proc/poky/meta-qti-bsp/recipes-devtools/ptool/.
```
### Apply Patch (in the docker)
Find the patch in the Docker:
```
cd  /home/turbox/lu.um.3.3.1/apps_proc/src/vendor/qcom/proprietary/
```
Apply the patch:
```
git apply Patch/0001-Camera-e2vTopaz2M-bring-up.patch --whitespace=nowarn
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
./turbox_build.sh -ul
```
```
./turbox_build.sh --zip_flat_build -l
```

