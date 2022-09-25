# Two Hours One Life

An Open Source game forked from One Hour One Life. We have no affiliation with the original game.

[![DigitalOcean Referral Badge](https://web-platforms.sfo2.digitaloceanspaces.com/WWW/Badge%203.svg)](https://www.digitalocean.com/?refcode=930cfa370b47&utm_campaign=Referral_Invite&utm_medium=Referral_Program&utm_source=badge)

## Project overview
The main project is separated into three repositories:
- OneLife: client, data editor and server (This repo!)
- minorGems: custom game engine
- OneLifeData7: game data

And unique to 2HOL as compared to upstream or other forks, we have separated additional feature servers into a fourth repository known as:
- OneLifeWeb: additional server features, individual web servers

The stack is majority C++ and PHP built for Windows and Linux.


## Contents
```
OneLife/
├── build/
├── commonSource/
├── documentation/
├── gameSource/
├── scripts/
└── server/
```

### build/
Scripts and assets used for building and packaging clients.
Building for some time now usually involves using a separate helper repository which then interacts with these scripts.

See the building section for more information.

### commonSource/
Small amount of files shared for building of the client and server.

### documentation/
Contains a lot of legacy information from the upstream project that is not necessarily accurate and largely not used by this project.

There is valuable information to be found here, but keep the above in mind.

Notably, 
- the website for the upstream project can be found in `html/`; we have partially migrated some of this to OneLifeWeb, but otherwise is not in use.
- `changeLog.txt` is notoriously kept up to date in the upstream project, but this is not a practice we have kept with. Later changes are sometimes merged and frequently cause conflicts in this file. Due to that, **it's history is not accurate and does not contain reference to any changes made in this fork.**

### gameSource/
Source directory of the game client and data editor client or "The Editor".

### scripts/
Scripts generally not used by this project; They do contain valuable information on how the upstream project manages different processes.

### server/
Source directory of the game server.

# Building

# License

# Contributing

Our community is centred around our Discord server, please join via our [website](https://twohoursonelife.com) to contribute. Use the `!programming` command to find the right people.