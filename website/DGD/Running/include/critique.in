From: dgd at list.imaginary.com (Felix A. Croes)
Date: Tue Feb  3 06:09:00 2004
Subject: [DGD] Call for critique:  LPC documentation

Here is an editor quick reference page I wrote for a mud I used to
work on, and which I made available using the 'h' command.

Regards,
Dworkin

                        Editor Quick Reference

          --LINES--                           --REGULAR EXPRESSIONS--
.       current line                    .       any single character
$       last line                       ^   $   start of line, end of line
5+7     line 12                         [a-z]   any character in range a .. z
/pat/   first line after current        *       zero or more occurances of
        in which pat appears                    preceding character
?pat?   like /pat/, but searching       \<  \>  start of word, end of word
        backwards
                              --COMMANDS--
(.)     a         append lines after .  (.,.) #         show numbered lines
(.,.)   c         change lines          (.,.) p         show lines
(.,.)   d         delete lines                q         quit edit session
        e file    edit file             (.,.) s/pat/str substitute pat by str
        f         show current file     (.,.) t line    copy lines
(1,$)   g/pat/cmd run cmd on lines            u         undo last change
                  matching pat          (1,$) w file    write to file
(.)     i         insert lines before .       x         write changes and quit
(.,.+1) j         join lines            (.+1) z         show page of lines
(.,.)   m line    move lines            (1,$) I         indent LPC code
