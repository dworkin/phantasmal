From: "Christopher Allen"
Subject: Re: [DGD] SkotOS 2.0
Date: Tue, 31 Aug 2004 12:40:32 -0700

Par Winzell wrote:
> It's not interpreted in LPC. Merry is compiled into actual LPC. There
> is no significant speed loss. The benefits are not so much in the
> language itself but in the fact that it allows little snippets of
> code to be sent around as data, which is a natural way for us to hook
> logic onto events.

The name Merry comes from "Mediated Execution Registry Involving A =
Dialect Of C" -- which although somewhat a joke because it is the =
successor of Bilbo "Built In Language Between Object", but the acronym =
that Merry stands for actually does describe it well.

The key is the the "mediated" part -- this means that Merry is somewhat =
sandboxed -- not to the scale that Java is, but helpful. For instance, =
in Merry code we try to ensure that Merry scripts inside game objects =
only interact with objects that involve the virtual game world, not =
random other objects it has no business with. Only a limited number of =
functions can be called from Merry, and even then they are only allowed =
implicitly, like fetching/setting properties, Act(), etc. File system =
access is completely disabled, as well as things like starting things up =
or shutting down processes, and some communications functions.

In addition, the langage is run through a complex parse_string() =
grammar, which gives us the ability to add some useful synatactic sugar =
additions: constants, per-thread global $variables, $foo: <expr> =
function-parameters, you can directly access the property database for =
values through ${someobject}.property:name, and a few other useful =
things like the ability to do inline SAM (Skotos Active Markup) with =
$"{oneof:one|two|three}".

Independent of the Merry language itself, there are a large variety of =
useful game specific functions, such as EmitTo(), EmitIn(), Set(), etc.

Here is a small code example. A simple torch, which responds (i.e. the =
"react-") to the signal (what might be called an event) named =
"react-post:light-dob". This signal is sent to a direct object (i.e. =
-dob) immediately after (i.e. -post) a verb is used that might turn on a =
light in an object (such as the verbs light, ignite, etc.):
/* Standard Flame Scripts   */  =20
/* react-post:light-dob     */  =20
/* Example by ChristopherA  */  =20
     =20
   /* If flame-on was set during the reaction, turn trait:flame on */  =20
   /* and let setprop-post:trait:flame handle cleaning things up.  */  =20
   /* Note: we do this in react-post so that things look clean     */  =20
   /* during normal processing of the react:light-dob signal,      */  =20
   /* e.g. you don't get the result "You light the flaming torch." */  =20
   /* when it isn't actually flaming yet.                          */   =
if ( Get(this,"trait:flame-on") ){      =20
      Set( this, "trait:flame", TRUE);      =20
      Set( this, "trait:flame-on", FALSE);=20
      /* Now show everyone that a flame has appeared.              */    =
 =20
      EmitTo ($actor, "The top of " + Describe($this, $actor, $actor) + =
" crackles alight.\n");    =20
      EmitIn(Get($actor, "base:environment"), "The top of " + =
Describe($this) + " crackles alight.\n", $actor);=20
   }  =20
  =20
   /* This is a react-post, so no need to return TRUE; */Note the =
$actor, $this -- these are per-thread global variables passed to this =
script. They will be the same for all the signals that were initially =
caused by the first "light my torch" verb.

Next we have a signal that is sent to objects if a property is changed, =
called setprop-post. An example of use is when the torch is lit -- =
either someone interacted with the torch directly (for instance "light =
my torch" above) or something else lit it (put my torch in fireplace) -- =
either way, the trait:flame property is set to true and =
setprop-post:trait:flame is sent to the object.

/* Standard Flame Scripts    */
/* setprop-post:trait:flame  */
/* Example by ChristopherA   */

   if ($(hook-property) !=3D "trait:flame") return TRUE;

   /* If a flame exists...   */
   if ( Get(this,"trait:flame") ){
      /* reveal the flame and change adjectives on the prime detail */
      Set( this, "details:flame:hidden", FALSE);
      Set( this, "details:smoke:hidden", FALSE);
      Set( this, "details:default:adjectives:flaming", TRUE);
      Set( this, "details:default:adjectives:lit", TRUE);
      Set( this, "details:default:adjectives:unlit", FALSE);

      /* if the torch has never been flamed before, scorch it.     */
      if ( ! (Get(this,"trait:flame:remains")) ) {
         Set(this, "trait:flame:remains", "It is slightly scorched.");
      }

      /* Now start the timer which will tick ever 90 seconds      */
      Set(this, "trait:flame:tick_id", Every("tick", 90));

   }
   /* ... if a flame does not exist, then hide the flame and      */
   /* change adjectives on the prime detail.                      */
   else {
      Set( this, "details:flame:hidden", TRUE);
      Set( this, "details:smoke:hidden", TRUE);
      Set( this, "details:default:adjectives:flaming", FALSE);
      Set( this, "details:default:adjectives:lit", FALSE);
      Set( this, "details:default:adjectives:unlit", TRUE);

      /* Now turn of the timer, so it will not tick. */
      Stop(Get(this, "trait:flame:tick_id"));
   }

   /* This is a setprop-post, so no need to return TRUE; */Finally, we =
need a way to emit messages periodically as the torch is burning, and to =
destroy the torch when it is used up:

/* Standard Flame Scripts    */=20
/* timer:tick                */=20
/* Example by ChristopherA   */=20
=20
   /* timer:tick is called every X seconds (typically 60 or */=20
   /* 90 seconds) as set up in the Every() function in      */=20
   /* the setprop-post:trait:flame merry script.            */=20
=20
   object env; object act; int tick;=20
=20
   /* We've been triggered, so increment our tick count     */=20
   tick =3D Get(this, "trait:flame:tick_cnt") + 1;=20
   Set(this, "trait:flame:tick_cnt", tick);=20
=20
   /* set some variables because timer:tick exists in an    */=20
   /* an environment where there are very few arguments     */=20
   /* and you can't rely on where the item. It could be     */=20
   /* on the floor, in someones bag in the nil, or in       */=20
   /* the hands of the person who lit it.                   */=20
=20
   /* If we are being held, we want our environment output  */=20
   /* to be in the room of the person who is holding us.    */

   if (env =3D Get(this, "base:environment")) {=20
    env =3D Get(env, "base:environment");=20
   }=20
=20
   /* If we are being held, we want our actor output        */=20
   /* to be the person holding us, or the room if the item  */=20
   /* has been dropped.                                     */

   act =3D this."base:environment";=20
   if (act."base:environment") {=20
      /* actor has an environment, all is well */=20
      env =3D act."base:environment";=20
   } else {=20
      /* if act has no environment, we've been dropped and act =3D env! =
*/=20
      env =3D act;=20
      act =3D nil;=20
   }=20
=20
   /* For each tick, do different things...                 */=20
=20
   /* ...partially burn out the item                        */=20
   if (tick =3D=3D 5) {=20
     Set(this, "trait:flame:remains", "It is about half burned out");=20
   }=20
   /* ...mostly burn out the item                           */=20
   if (tick =3D=3D 10) {=20
      Set(this, "trait:flame:remains", "It is almost burned out.");=20
   }=20
   /* ...complete burn out the item                         */=20
   if (tick >=3D 12) {=20
      if ( act !=3D nil ) {=20
          EmitTo(act, Describe(this, nil, act) + " sputters out.\n");=20
      }=20
      if(this."base:environment") {=20
         EmitIn(env, Describe(this, nil, nil) + " sputters out.\n", =
act);=20
      }=20
      Set(this, "trait:flame", FALSE);=20
      Set(this, "trait:flame:flammable", FALSE);=20
      Set(this, "trait:flame:remains", "It is completely burned out.");=20
      Set( this, "details:default:adjectives:spent", TRUE);=20
      /* stop us from ticking anymore                       */=20
      Stop(Get(this, "trait:flame:tick_id"));=20
=20
      /* Now return false, but set up a delay so that       */=20
      /* this object can decay in 86400 seconds (1 day)     */=20
      $delay(86400, FALSE, "4a8d");=20
=20
      /* We have to re-establish our local variables, as    */=20
      /* they are incorrect after a delay.                  */=20
      Set(this, "trait:flame:tick_cnt", tick);=20
      if (env =3D Get(this, "base:environment")) {=20
    env =3D Get(env, "base:environment");=20
      }=20
      act =3D this."base:environment";=20
      if (act."base:environment") {=20
         env =3D act."base:environment";=20
      } else {=20
         env =3D act;=20
         act =3D nil;=20
      }=20
=20
      /* If we are not in the nil, then tell people that    */=20
      /* we are disintegrating...                           */=20
      if ( act !=3D nil ) {=20
        EmitTo(act, Describe(this, nil, act) + " disintegrates from =
age.\n");=20
     }=20
      if (this."base:environment") {=20
         EmitIn(env, Describe(this, nil, nil) + " disintegrates from =
age.\n", act);=20
     }=20
    /* Ok, we are done. Bye.                              */=20
      Slay(this);=20
      return FALSE;=20
   }=20
=20
   /* For every tick, there is a 30% chance of a random emit*/=20
   switch(random(10)) {=20
      /* Again, if we are not in the nil, do the emits      */=20
    case 0: {=20
            if ( act !=3D nil ) {=20
               EmitTo(act, "A trail of smoke wisps upwards through the =
air from " + Describe(this, nil, act) + ".\n");=20
            }=20
            if(this."base:environment") {=20
            EmitIn(env, "A trail of smoke wisps upwards through the air =
from " + Describe(this, nil, nil) + ".\n", act);=20
            }=20
    } break;=20
    case 1: {=20
            if(this."base:environment") {=20
               EmitTo(act, "There is a quiet fizzle of hot oil from " + =
Describe(this, nil, act) + ".\n");=20
            }=20
            if(this."base:environment") {=20
               EmitIn(env, "There is a quiet fizzle of hot oil from " + =
Describe(this, nil, nil) + ".\n", act);=20
            }=20
    } break;=20
    case 2: {=20
            if(this."base:environment") {=20
               EmitTo(act, capitalize(Describe(this, nil, act) + " =
flickers.\n"));=20
            }=20
            if(this."base:environment") {=20
               EmitIn(env, capitalize(Describe(this, nil, nil) + " =
flickers.\n"), act);=20
            }=20
    } break;
    default: break;
   }

The above is only three of about 8 different Merry scripts used by =
torches.

-- Christopher Allen

------------------------------------------------------------------------
.. Christopher Allen                                 Skotos Tech Inc. ..
..                2342 Shattuck Ave Ste #512, Berkeley, CA 94704-1517 ..
.. www.skotos.net  www.rpg.net       o510/647-2760   f510/849-1717 ..
