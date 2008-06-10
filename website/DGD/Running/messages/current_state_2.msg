From: DGD Mailing List (Stephen Schmidt)
Date: Mon Aug 23 21:55:02 2004
Subject: [DGD] Current state of MUD-dom

On Mon, 23 Aug 2004, David Jackson wrote:
> 2)  We need OLC creation tools, to inspire non-coders to become builders...

Most MUD thinking has been assuming that builders have to be
wizards, ie, have file access. This isn't necessarily true.
I was on a mud that did a brief experiment with player-built
rooms. Players could form clans, and clans got houses which
came with a room tool that they could use to design their
house to their own specs. Most got pretty elaborate even
though it was totally outside the game system.

The version I'd do now, if I had it to do again, is this: When
a player makes level 20, they become a lord and they get a
castle. They can build it as they like. If they put in useful
services (shops, taverns, whatever fits the game world) and
enough players spend enough time in their castle, they get
rewarded for that - more resources to build bigger, something.
Then the lords compete among one another in world development,
who can have the biggest city, strongest army, most player
traffic through their town, whatever. "Castle" need not be
taken too literally; dwarves build cave systems, elfs build
forest towns, whatever. Then players in the dwarf caves can
compete against the orc players in their caves. Build in
enough player rivalry, and you need neither rooms nor NPCs;
players do everything. (At least, that's the goal. Actual
implementation or even design of this is at the epsilon stage.)

> 4)  We need a stock lib with enough playable content so that the average
> guy will be inspired to set up a MUD in the first place...

2.4.5 had this. It was unpopular with driver coders: if you hand
out playable content, everyone who sets up a mud uses that playable
content, so the first 30 rooms (or however much content you give)
are identical on all muds. (Who else remembers the humpbacked
bridge?)

Also, the pre-existing playable content defines a theme (it
can't do otherwise) and then everyone gravitates into that
theme.

TMI-2 very deliberately didn't include -any- playable content
(the few samples rooms that had to be there were made unusable
through idiosyncracy). The philosophy was, TMI-2 gives you the
tools, what to do with them us your job. The decision was not
popular. On the other hand, the people who used TMI-2 were
generally people who did have some original content thoughts
and could implement them. Nightmare lib, released about the
same time for the same driver, including a reasonable amoung
of playable content, had far more people downloading it, but
(IMHO) less variety in what those users did with it.

Steve
