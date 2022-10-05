# Two Hours One Life

![](https://opencollective.com/twohoursonelife/tiers/badge.svg)

---

**An Open Source game, forked from One Hour One Life.**

From One Hour One Life:
> A multiplayer survival game of parenting and civilization building. Get born to another player as your mother. Live an entire life in one hour. Have babies of your own in the form of other players. Leave a legacy for the next generation as you help to rebuild civilization from scratch.

In addition 2HOL offers the ability to live twice as long, focusing on building a community in and outside the game, rather than specifically focusing on short lived families and towns, our maps last in excess of 6 months and ideally longer. Players have the ability to repeatedly spawn at their own individual spawn points, which they can choose to share with their friends or alternatively they can specify the name of the family they wish to spawn as a child in.

We're community oriented and continually take on feedback. We release multiple content updates a year and continue to add suggested features while progressing different play styles and tech.

The community is what makes this project what it is.

## Project overview
### A brief history
Beginning shortly after the release of One Hour One Life (Feb 2018) as a semi-private server, twohoursonelife.com began as dying.world and integrated the name Two Hours One Life in mid 2018. The 14th of March 2018, is regarded as the birth date of 2HOL. Two Hours One Life, shortened as "2HOL".

As has always been, the core difference was the double in life length, mixed with a fixed spawn point this allowed players to return to their towns over several lives and weeks, enabling a slower play style, community connection and role playing.

Fast forward to today, modern 2HOL last branched off in August 2019 and has since independently developed key features and content whilst continuing to cherry-pick whole and parts of features and bug fixes from the original project.

2HOL now features, among the core offering since inception, stringent moderation, systems to allow recurring spawning at fixed locations,

Playing 2HOL is entirely managed via our Discord guild, the home base of our community. Players can access their login credentials here by interacting with our Dictator bot.
### Structure
The main project is separated into three repositories:
- **OneLife**: client, data editor and server (You are here!)
- **minorGems**: custom game engine
- **OneLifeData7**: game data

And unique to 2HOL as compared to upstream or other forks, we have separated additional feature servers into a fourth repository known as:
- **OneLifeWeb**: additional server features, individual web servers

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

### `build/`
---
Scripts and assets used for building and packaging clients.
Building for some time now usually involves using a separate helper repository which then interacts with these scripts.

See the building section for more information.

### `commonSource/`
---
Small amount of files shared for building of the client and server.

### `documentation/`
---
Contains a lot of legacy information from the upstream project that is not necessarily accurate and largely not used by this project.

There is valuable information to be found here, but keep the above in mind.

Notably, 
- the website for the upstream project can be found in `html/`; we have partially migrated some of this to OneLifeWeb, but otherwise is not in use.
- `changeLog.txt` is notoriously kept up to date in the upstream project, but this is not a practice we have kept with. Later changes are sometimes merged and frequently cause conflicts in this file. Due to that, **it's history is not accurate and does not contain reference to any changes made in this fork.**

### `gameSource/`
---
Source directory of the game client and data editor client or "The Editor".

### `scripts/`
---
Scripts generally not used by this project; They do contain valuable information on how the upstream project manages different processes.

### `server/`
---
Source directory of the game server.

# Building

Three applications are cross compiled from Linux for the core project. The game client, editor and server. It can be configured to build for Windows, Linux and Mac (Although Mac has not been supported since MacOS 10.15 and neither OHOL or 2HOL have released a Mac client since.)

The core scripts to build clients are found in the `build/` directory. It is common place in OHOL and 2HOL communities to use a third party collection of scripts to help interface with these core scripts. A number of people have made collections of scripts to do this over the years.

2HOL currently recommends the use of [risvh/miniOneLifeCompile](https://github.com/risvh/miniOneLifeCompile) and is progressing the conversion and abilities of [twohoursonelife/build-tools](https://github.com/twohoursonelife/build-tools/)

# License

The core project is released into the public domain, this includes all contents within the repositories OneLife, OneLifeData7, minorGems and OneLifeWeb.

Please see `no_copyright.txt` for the full text.

# Contributing

Our community is centred around our Discord server, please join via our [website](https://twohoursonelife.com) to contribute. 

While code is one way to contribute, it certainly is not the only one. We also greatly contributions in the form of project management, art production, event management and other forms of support.

Due to the nature of an open source volunteer project, it is resource intensive for us to on board new contributors. While we welcome everyone, please do keep this in mind as this will unnecessarily divert resources from development. You can put your best foot forward by becoming experienced in playing the game and making yourself familiar in our Discord guild.

We appreciate small pull requests to fix minor issues, but please open an issue or begin a discussion on GitHub or Discord before investing time into a larger pull request.

# Support
The project is supported by community donations, and developed entirely voluntarily. Please consider donating on Open Collective.

[<img src="https://opencollective.com/twohoursonelife/donate/button@2x.png?color=blue" width=300 />](https://opencollective.com/twohoursonelife/donate)


You can also support our costs by using our DigitalOcean referral link.

[![DigitalOcean Referral Badge](https://web-platforms.sfo2.cdn.digitaloceanspaces.com/WWW/Badge%201.svg)](https://www.digitalocean.com/?refcode=930cfa370b47&utm_campaign=Referral_Invite&utm_medium=Referral_Program&utm_source=badge)

---
*2HOL is in no way affiliated with the original game or it's producers.*
