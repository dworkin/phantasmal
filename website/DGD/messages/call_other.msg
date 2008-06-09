From: dgd@dworkin.nl (Par Winzell)
Date: Tue May 10 18:25:01 2005
Subject: [DGD] call_other()

So... in traditional DGD/LPC,

  object o;

  o = new_object(ARRAY_LIST);
  o->add("foo");

Meanwhile, in Java:

  List<String> l = new ArrayList()<String>;
  l.add("foo");


These codelets look similar but there's at least one huge difference
between them.

In LPC, we're sending object-name strings into functions. In Java, the
class names are part of the syntax of the language. In LPC, we won't
know until runtime whether or not ARRAY_LIST even exists, if it can be
instantiated, and if it can, whether or not 'add' exists, if it can
accept a string... etc.

Java, by contrast, will not compile this codelet without also compiling
List and ArrayList. It will refuse to compile the codelet if ArrayList
cannot be instantiated in this way, and it will refuse to compile a call
a method that's not defined in the List interface.


Of course, Java is not in competition with LPC. Java does not easily
recompile on the fly. Excepting debugging extensions and tricky
classloader stuff, a JVM is meant to be restarted for code changes to
take effect. DGD in contrast offers genuine persistence natively and
in-place recompilation is one of the most basic tools we need to keep
our decade-long uptimes sane.

So there is no virtue in attempting to become as Java-like as possible.
That said, the additional typechecking available to the Java code above
is invaluably pleasant. Wouldn't it be nice if we could get both? Let's
see how far we could get if we were to launch into this insane project.

 - We'd definitely have to redefine compile_object(), sending supplied
LPC source through a parse_string() grammar of our own making. We mostly
collect meta-data during compilation, but also enforce restrictions, as
well as modify the syntax of the LPC we accept.
 - Delete call_other() completely, and do not replace it. We accept the
-> operator on string constants and object-type objects only. The name
of the function to be called can no longer be variable. As in Java
(ignoring reflection), it must be explicitly and statically part of the
source code.
 - Just as we currently store inheritance dependency information for
every program we see compiled, we're going to have to now store type
information for every function of every program and even every argument
of every such function.
 - During parsing, tracks the static type of every value, including
return values from other functions (using DB mentioned in previous
line). If we encounter code like,

  void kill(object LIVING foo) {
    foo->die();
  }
or perhaps
  void create() {
    VIRUS_DB->register_new_infection(this_object());
  }
and we're parsing the function call, LIVING/VIRUS_DB must already have
been compiled at this point. We either error if it hasn't, or we force a
compile ourselves.... not sure. Then look up e.g. 'die' for LIVING in
the DB, (error if it doesn't exist), and type-check each argument as far
as we're able and willing.

When we recompile a program P now, we calculate a set of other classes
that have to be recompiled/destroyed because they depend on P through
inheritance. We'd need to extend this to inter-object call dependencies
as well. Thus recompiling LIVING or VIRUS_DB would trigger a recompile
of the codelets above.

If there's one real problem, it's the fact that DGD's preprocessor is
not exposed, and unless we implement the entire damn thing ourselves, we
never get to see the LPC in an expanded, post-preprocessor state. There
might be some way to hack around this, but it's very frustrating.

So, anyway, apart from that. It's a bit of work to be sure, but can
anybody shoot holes in this? It looks to me like it could work, and we'd
effectively have static compile-time type checking of inter-object
function calls, without sacrificing any of DGD's vital recompilation
abilities.

The way we write LPC would need to change, of course. We're quite used
to being able to call non-existant functions on objects, allowing those
objects to implement the functions if they should. We're used to doing
things like filter_array() and map_array(). Perhaps losing the ability
to call variable functions without having something quite like Java's
anonymous classes is too big a loss. But tackling anonymous classes is a
subject for another day... :)




Thoughts? Am I insane? :)

Zell
