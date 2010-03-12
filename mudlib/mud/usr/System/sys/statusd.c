#include <kernel/kernel.h>
#include <kernel/user.h>

#define SPACE16 "                "

inherit LIB_USER;

static create()
{
	USERD->set_binary_manager(3, this_object());
}

private string ralign(mixed num, int width)
{
    string str;

    str = SPACE16 + (string) num;
    return str[strlen(str) - width ..];
}

private string swapnum(int num, int div)
{
    string str;

    str = (string) ((float) num / (float) div);
    str += (sscanf(str, "%*s.") != 0) ? "00" : ".00";
    if (strlen(str) > 4) {
	str = (str[3] == '.') ? str[.. 2] : str[.. 3];
    }
    return str;
}

private string status()
{
	mixed *status;
	string str;
	
	status = status();
	str =
"                                          Server:       " +
  (string) status[ST_VERSION] + "\n" +
"------------ Swap device -------------\n" +
"sectors:  " + ralign(status[ST_SWAPUSED], 9) + " / " +
	       ralign(status[ST_SWAPSIZE], 9) + " (" +
  ralign((int) status[ST_SWAPUSED] * 100 / (int) status[ST_SWAPSIZE], 3) +
  "%)    Start time:   " + ctime(status[ST_STARTTIME])[4 ..] + "\n" +
"sector size:   " + (((float) status[ST_SECTORSIZE] / 1024.0) + "K" +
		     SPACE16)[..15];
	if ((int) status[ST_STARTTIME] != (int) status[ST_BOOTTIME]) {
	    str += "           Last reboot:  " +
		   ctime(status[ST_BOOTTIME])[4 ..];
	}

	uptime = status[ST_UPTIME];
	seconds = uptime % 60;
	uptime /= 60;
	minutes = uptime % 60;
	uptime /= 60;
	hours = uptime % 24;
	uptime /= 24;
	short = status[ST_NCOSHORT];
	long = status[ST_NCOLONG];
	i = sizeof(USERD->query_connections());
	str += "\n" +
"swap average:  " + (swapnum(status[ST_SWAPRATE1], 60) + ", " +
		     swapnum(status[ST_SWAPRATE5], 300) + SPACE16)[.. 15] +
  "           Uptime:       " +
  ((uptime == 0) ? "" : uptime + ((uptime == 1) ? " day, " : " days, ")) +
  ralign("00" + hours, 2) + ":" + ralign("00" + minutes, 2) + ":" +
  ralign("00" + seconds, 2) + "\n\n" +
"--------------- Memory ---------------" +
  "    ------------ Callouts ------------\n" +
"static:   " + ralign(status[ST_SMEMUSED], 9) + " / " +
	       ralign(status[ST_SMEMSIZE], 9) + " (" +
  ralign((int) ((float) status[ST_SMEMUSED] * 100.0 /
		(float) status[ST_SMEMSIZE]), 3) +
  "%)    short term:   " + ralign(short, 5) + "         (" +
  ((short + long == 0) ? "  0" : ralign(100 - long * 100 / (short + long), 3)) +
  "%)\n" +
"dynamic:  " + ralign(status[ST_DMEMUSED], 9) + " / " +
	       ralign(status[ST_DMEMSIZE], 9) + " (" +
  ralign((int) ((float) status[ST_DMEMUSED] * 100.0 /
	 (float) status[ST_DMEMSIZE]), 3) +
  "%) +  other:        " + ralign(long, 5) + "         (" +
  ((short + long == 0) ? "  0" : ralign(long * 100 / (short + long), 3)) +
  "%) +\n" +
"          " +
  ralign((int) status[ST_SMEMUSED] + (int) status[ST_DMEMUSED], 9) + " / " +
  ralign((int) status[ST_SMEMSIZE] + (int) status[ST_DMEMSIZE], 9) + " (" +
  ralign((int) (((float) status[ST_SMEMUSED] +
		 (float) status[ST_DMEMUSED]) * 100.0 /
		((float) status[ST_SMEMSIZE] +
		 (float) status[ST_DMEMSIZE])), 3) +
  "%)                  " + ralign(short + long, 5) + " / " +
			   ralign(status[ST_COTABSIZE], 5) + " (" +
  ralign((short + long) * 100 / (int) status[ST_COTABSIZE], 3) + "%)\n\n" +
"Objects:  " + ralign(status[ST_NOBJECTS], 9) + " / " +
	       ralign(status[ST_OTABSIZE], 9) + " (" +
  ralign((int) status[ST_NOBJECTS] * 100 / (int) status[ST_OTABSIZE], 3) +
  "%)    Connections:  " + ralign(i, 5) + " / " +
			   ralign(status[ST_UTABSIZE], 5) + " (" +
  ralign(i * 100 / (int) status[ST_UTABSIZE], 3) + "%)\n\n";
  
  return str;
}

int query_timeout(object connection)
{
	return 5;
}

object select(string request)
{
	return this_object();
}

int login(string request)
{
	string status;
	
	connection(previous_object());
	
	status = status();
	message(status);
	
	return MODE_DISCONNECT;
}
