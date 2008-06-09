From: DGD Mailing List (Christopher Allen)
Date: Thu Mar 25 10:36:01 2004
Subject: [DGD] Aliases & Stacked commands

Michael McKiel wrote:
> Shoulda asked in my last reply :P but anyways...
> I'm not sure I get the command/action distinction as defined by
> Skotos.
>
> As I previously stated, any object can potentially try to do a given
> command, and for most cases will be able to, non-livings limited to
> souls & 'talking' commands. And mobiles restricted from wiz-commands
> (that might change but don't think it should be needed.)
>
> Isn't the action(if we exclude wizard commands) primarily the message
> displayed from a given command, or a movement of one object into
>    another, ie: get/give/drop, move to another room, etc.
>
> Or does skotos make some other distinction that I am missing?

I'll give a specific example, the verb sip:

<object id="OBJ(Socials:Verbs:S:sip)">
  <context/>
  <Socials:Verb imp="sip" second="sip" third="sips" evoke="forbidden"
audible="false" private="false" secret="false" obscured="false"
target-abstracts="false" disabled="false">
    <Socials:SocialObjects>
      <Socials:SocialObject role="dob" direct="true" single="false" raw="false"
requirement="optional" distance="close"/>
      <Socials:SocialObject role="iob" direct="false" single="false" raw="false"
requirement="optional" distance="close">
        <Socials:SocObPreps>
          <Socials:SocObPrep prep="from"/>
        </Socials:SocObPreps>
      </Socials:SocialObject>
    </Socials:SocialObjects>
    <Socials:VerbActions>
      <Socials:VerbAction action="sense/taste"/>
      <Socials:VerbAction action="act/consume"/>
      <Socials:VerbAction action="act/drink/small"/>
    </Socials:VerbActions>
  </Socials:Verb>
</object>

Basically when someone sips a drink, the action "sense/taste" is triggered,
sending the signal "sense/taste" to all the appropriate nearby objects. This
allows the object to tell the actor "You taste a blend of warm milk and dark
bitter-sweet chocolate with a dash of cinnamon", but also could be used by the
actor to detect a poison. Next is the "act/consume" signal, which can be used by
objects to know that something was consumed, for instance, to notice that
someone was eating in the rectory, or to trigger the poison. Next, there is
"act/drink/small" which consumes a small amount of the liquid, as opposed to
guzzling it. Finally, there is a default "verb/sip" signal, for those odd cases
where use of the specific verb is significant.

Compare this to taste:

<object id="OBJ(Socials:Verbs:T:taste)">
  <context/>
  <Socials:Verb imp="taste" second="taste" third="tastes" evoke="forbidden"
audible="false" private="false" secret="false" obscured="false"
target-abstracts="false" disabled="false">
    <Socials:SocialObjects>
      <Socials:SocialObject role="dob" direct="true" single="false" raw="false"
requirement="optional" distance="close"/>
      <Socials:SocialObject role="iob" direct="false" single="false" raw="false"
requirement="optional" distance="close">
        <Socials:SocObPreps>
          <Socials:SocObPrep prep="from"/>
        </Socials:SocObPreps>
      </Socials:SocialObject>
    </Socials:SocialObjects>
    <Socials:VerbActions>
      <Socials:VerbAction action="sense/taste"/>
    </Socials:VerbActions>
  </Socials:Verb>
</object>

When someone tastes a drink, only "sense/taste" and "verb/taste" signals are
triggered, but not "act/consume" or any "act/drink". Thus the taste of the
mexican hot chocolate is given to the user, and the user could have a chance of
detecting the poison, but the poison itself would not be consumed (unless it was
so powerful that it was triggered by "sense/taste" in addition to
"act/consume").

There are several thousand of verbs in our library, but as a coder you don't
have to worry about the distinctions between consume, drain, gargle, gulp,
guzzle, imbibe, lap, lick, quaff, sip, slurp, suck, swig, swallow, swill, taste,
etc. You only have to code to "sense/taste", "act/consume", "act/drink".

It is also trivial for people to to add additional actions to a verb, for
instance, "act/touch" could be added to the verb "sip" and all the verbs where
touch is assumed, and now this signal would be available to players.

In addition to the verb signals, other signals can be sent, for instance, when
an NPC moves, there are signals sent to all who could observe it. If the lights
go off, a signal is sent. If someone becomes hungry, a signal is sent.

-- Christopher Allen
