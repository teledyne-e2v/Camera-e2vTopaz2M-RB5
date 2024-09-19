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
## Start SDK manager 
