From: dgd at list.imaginary.com (Christopher Allen)
Date: Sun Feb  2 20:03:01 2003
Subject: Re[2]: [DGD] Impressed with DGD

"Christopher Darque":
> > If you are considering a game for us, either using your own library
>
> First let me say thanks for the effort of cutting and pasting
> all that info, I appreciate it and I have saved it for serious
> study. I am fairly familiar with HTML and Pueblo, but I know
> very little about ActiveX.

You don't really need to know about the clients, just how to interface
with them. For instance, in our Marrach client, when an object is
dropped in a room, it also shows up in a popup menu to the right of the
of the text area. The way this works is that whenever an object arrives
in a room, a SKOOT event is sent, basically something like "SKOOT 15 a
rock". The web page recieves the SKOOT 15 messages, and sends it to
function 15, which in our case is the javascript:

function addObject(str) {
  var ix, pos, name;

  pos = str.indexOf(' ');
  if (pos >= 0) {
    ix = str.substring(0, pos);
    name = str.substring(pos+1);
    if (this["objectSet"] == null) {
        this["objectSet"] = new Object();
    }
    this["objectSet"][ix] = name;
    updateAll();
//    alert('object ' + name + ' added');
  }
}

The fact that we use 15 for addObject is not relevant -- WAP uses a
completely different set of SKOOT # messages that call Javascripts that
they wrote for their client. Whatever you can do in Javascript
dynamically (i.e. without refreshing the page) can be done using this
technique.

> Second, where might I find more technical docs on the Skotos
> systems?  If I do decide to propose this to your Company I
> would almost certainly use your Lib as the starting point.

There is quite a bit of documentation that you get when you are involved
with developing for us, however, I'd say though we have alot, is never
enough ;-)

> Your use of non-telnet clients does open up some interesting
> possibilities, such as a split screen with the room visual
> at the top and a scrolling text box below for everything
> else.

It is even possible to have the client be a 1x1 pixel client, and just
use it to control the dynamic web page.

-- Christopher Allen

------------------------------------------------------------------------
.. Christopher Allen                                 Skotos Tech Inc. ..
..                           1512 Walnut St., Berkeley, CA 94709-1513 ..
.. <http://www.skotos.net>           o510/647-2760x202  f510/647-2761 ..
