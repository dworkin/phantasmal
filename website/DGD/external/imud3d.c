/**************************************************************************
 *                                                                        *
 * imud3d.c - An InterMUD 3 daemon object for DGD v1.1                    *
 * By Oojmaflip@Wyrd Dreams                                               *
 *                                                                        *
 * Please note:                                                           *
 *                                                                        *
 *    this code was written for DGD 1.1 with the comment patch, the       *
 * ANSI patch and the networking patch installed.  The first two are not  *
 * essential for the operation of this code if you're willing to pick     *
 * through the code and remove the bits that aren't relevent.  The latter *
 * is ESSENTIAL for this code to work.  Don't even bother to try it if    *
 * you don't have the networking patch installed, DGD doesn't have the    *
 * ability to open network sockets without it so there is no chance of it *
 * working.  Just to save you some hassle.  *8O)                          *
 *                                                                        *
 * Disclaimer:                                                            *
 *                                                                        *
 *    I'm no expert on DGD, LPC, IMUD3 or networking in general.  If you  *
 * use this code and it messes up or gets you into trouble somehow, it's  *
 * not my responsibility.  By using this code, you agree that any damage  *
 * it may cause is your own responsibility and nothing at all to do with  *
 * me.  Likewise, should anything really favourable happen as a result of *
 * you using this code, it's nothing to do with me.  But by all means     *
 * thank me when you're knocking Bill Gates off his block.  *8O)          *
 *    Any questions or queries should be directed to the following e-mail *
 * address, since the chances of me getting any MUDmail at Wyrd Dreams    *
 * are fairly remote.                                                     *
 *                                                                        *
 * James Tait <JTait-mud@wyrddreams.demon.co.uk>                          *
 *                                                                        *
 *************************************************************************/

# include <kernel/kernel.h>
# include <kernel/user.h>
# include <type.h>

string **router_list;          // list of routers, in order of preference
int password;                  // password, sent by router
int mudlist_id;                // MUD list ID, sent by router
mapping mudlist;               // MUD list, also sent by router
mapping local_mudlist;         // Local names for MUDs
int chanlist_id;               // channel list ID, sent by router
static string* buffer;         // packets to be sent
static string inBuffer;        // data buffer for incoming packets

string toMUDMode(mixed *args);          // converts an array to a packet
string parseArray(mixed *args);         // to convert an array
string parseMapping(mapping arg);       // converts a mapping
void sendPacket(string packet);         // sends an IMUD3 packet
void message_done();                    // all messages have been sent
void receive_message(string data);      // packet arrived from router
mixed *decodePacket(string data);       // decode packet from router
void handlePacket(mixed *packet);       // handle a data packet
void new_receive_message(string data);  // in-development receive-message


void create()
{
  string ipaddr;
  int portnum;

  find_object(DRIVER)->message("IMUD3 Daemon started.\n");
  restore_object("/kernel/data/imud3d.dat");
  if(!router_list)
    router_list = ({ ({ "*gjs", "208.192.43.105 9000" }) });
  if(!mudlist)
    mudlist = ([]);
  sscanf(router_list[0][1], "%s %d", ipaddr, portnum);
  connect(ipaddr, portnum);
  inBuffer = "";
}

/**************************************************************************
 *                                                                        *
 * Name:       tellAllUsers()                                             *
 * Purpose:    sends a message to all users                               *
 * Parameters: string msg - the message to be sent                        *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
void tellAllUsers(string msg)
{
  object *users;
  int i;

  users = users();
  for(i = 0; i < sizeof(users); i++)
  {
    if(sscanf(object_name(users[i]), DEFAULT_USER + "#%*d") != 0)
      users[i]->message(msg);
  }
}

/**************************************************************************
 *                                                                        *
 * Name:       toMUDMode()                                                *
 * Purpose:    converts an array of data into a MudOS MUDmode data packet *
 * Parameters: mixed *args - the array to be converted into a mudmode     *
 *                           data packet.                                 *
 * Returns:    string      - the mudmode data packet ready to be sent     *
 *                                                                        *
 *************************************************************************/
string toMUDMode(mixed *args)
{
  int len;     // the length of the packet
  int i;       // just used as a counter later on

  // The four spaces at the start of ret are for the packet size bytes
  string ret;
  ret = "    " + parseArray(args);

  // now we fill in the packet size bytes
  len = strlen(ret) - 4;
  for(i=3; i > -1; i--)
  {
    ret[i]=len & 255;
    len >>= 8;
  }

  return ret;
}

/**************************************************************************
 *                                                                        *
 * Name:       parseArray()                                               *
 * Purpose:    converts an array of data into its string representation   *
 * Parameters: mixed *args - the array to be converted                    *
 * Returns:    string      - string representation of the array           *
 *                                                                        *
 *************************************************************************/
string parseArray(mixed *args)
{
  int i;
  string ret;
  ret = "({";

  for(i=0; i<sizeof(args); i++)
  {
    switch(typeof(args[i]))
    {
      case T_INT:
      case T_FLOAT:     ret += args[i];
                        break;
      case T_STRING:    ret += "\"" + args[i] + "\"";
                        break;
      case T_ARRAY:     /*************************************************
                         *                                               *
                         * Note: since the IMUD3 protocol uses arrays of *
                         *       arrays, we make a recursive call here   *
                         *                                               *
                         ************************************************/
                        ret += parseArray(args[i]);
                        break;
      case T_MAPPING:   // handle a mapping
                        ret += parseMapping(args[i]);
                        break;
             default:   // throw up an error message here
                        error("Invalid type for IMUD3 packet\n");
    }
    ret += ",";
  }
  ret += "})";
  return ret;
}

/**************************************************************************
 *                                                                        *
 * Name:       parseMapping()                                             *
 * Purpose:    converts a mapping into its string representation          *
 * Parameters: mapping arg - the mapping to be converted                  *
 * Returns:    string      - string representation of the mapping         *
 *                                                                        *
 *************************************************************************/
string parseMapping(mapping arg)
{
  string ret;                          // to contain the converted mapping
  string *keystmp;                     // temporary store for converted keys
  string *valstmp;                     // same for values
  int i;                               // counter
  mixed *keys;                         // unconverted keys

  ret = "([";                          // initiate mapping

  // first we create an array of strings, keystmp, for the indices
  keys = map_indices(arg);
  keystmp = allocate(sizeof(keys));
  for(i=0; i < sizeof(keys); i++)
  {
    switch(typeof(keys[i]))
    {
      case T_INT:
      case T_FLOAT:     keystmp[i] = "" + keys[i];
                        break;
      case T_STRING:    keystmp[i] = "\"" + keys[i] + "\"";
                        break;
      case T_ARRAY:     keystmp[i] = parseArray(keys[i]);
                        break;
      case T_MAPPING:   keystmp[i] = parseMapping(keys[i]);
                        break;
             default:   // generate an error
                        error("Incompatible type for IMUD3 packet\n");
    }
  }

  // same again, this time for the values
  valstmp = allocate(sizeof(keys));
  for(i=0; i < sizeof(keys); i++)
  {
    switch(typeof(arg[keys[i]]))
    {
      case T_INT:
      case T_FLOAT:     valstmp[i] = "" + arg[keys[i]];
                        break;
      case T_STRING:    valstmp[i] = "\"" + arg[keys[i]] + "\"";
                        break;
      case T_ARRAY:     valstmp[i] = parseArray(arg[keys[i]]);
                        break;
      case T_MAPPING:   valstmp[i] = parseMapping(arg[keys[i]]);
                        break;
             default:   // generate an error
                        ("Incompatible type for IMUD3 packet\n");
    }
  }

  // right, now we have both the keys and values in string form, we 
  // concatenate the lot into one big string
  for(i=0; i < sizeof(keystmp); i++)
  {
    ret += keystmp[i] + ":" + valstmp[i] + ",";
  }

  // and finish it off by closing the mapping
  ret += "])";
  return ret;
}

/**************************************************************************
 *                                                                        *
 * Name:       open()                                                     *
 * Purpose:    called when a connection is made with the IMUD3 server     *
 * Parameters: none                                                       *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
void open()
{
  find_object(DRIVER)->message("IMUD3 port opened (" +
        query_ip_number(this_object()) + "), registering with server.\n");
  sendPacket(toMUDMode( ({ "startup-req-3",
                                         5,
                                         "Wyrd Dreams",
                                         0,
                                         router_list[0][0],
                                         0,
                                         password,
                                         mudlist_id,
                                         chanlist_id,
                                         8080,
                                         8090,
                                         8091,
                                         "WDLib 0.01 pre-alpha",
                                         "Custom",
                                         "DGD v1.1net",
                                         "LP",
                                         "MUDLib development",
                                         "JTait-mud@wyrddreams.demon.co.uk",
                                         ([ "tell":1, "emoteto":1, "who":0,
                                            "finger":0, "locate":0,
                                            "channel":1, "news":0,"mail":0,
                                            "file":0, "auth":0, "ucache":0,
                                            "smtp":0, "ftp":0, "nntp":0,
                                            "http":0, "rcp":0, "amcp":0 ]),
                                         ([ /* no other info */ ])
                                      }) ) );
}

/**************************************************************************
 *                                                                        *
 * Name:       close()                                                    *
 * Purpose:    called when the connection to the IMUD3 server is closed   *
 * Parameters: none                                                       *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
void close()
{
  find_object(DRIVER)->message("IMUD3 port closed.\n");
}

/**************************************************************************
 *                                                                        *
 * Name:       sendPacket()                                               *
 * Purpose:    sends a packet to the IMUD3 server                         *
 * Parameters: string packet - the IMUD3 packet to be sent                *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
void sendPacket(string packet)
{
  if(send_message(-1))
    buffer += ({ packet });
   else
    send_message(packet);
}

/**************************************************************************
 *                                                                        *
 * Name:       message_done()                                             *
 * Purpose:    called when all messages have been sent                    *
 * Parameters: none                                                       *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
void message_done()
{
  if(sizeof(buffer))
  {
    sendPacket(buffer[0]);
    buffer -= ({ buffer[0] });
  }
}

/**************************************************************************
 *                                                                        *
 * Name:       receive_message()                                          *
 * Purpose:    called when data arrives from the router                   *
 * Parameters: string data - the data packet sent by the router           *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
void receive_message(string data)
{
  mixed *elements;
  int packet_size, i;
//  string datatmp;

//  datatmp = data;
  packet_size = 0;
  // extract the packet size
  for(i=0; i<4; i++)
  {
    packet_size <<= 8;
    packet_size += (int)data[i];
  }
  // and remove it from the front of the packet
  data = data[4..strlen(data)-1];

  if(strlen(data) != packet_size)
  {
    write_file("/imud3.log", "Bad IMUD3 data packet.  Reported size: " + packet_size + " Actual size: " + strlen(data) + ".\n" + data + "\n\n\n");
    return;
  }
  // a little fix to remove any trailing zeroes
  while(!data[strlen(data)-1])
    data=data[0..strlen(data)-2];
  elements = decodePacket(data);
  handlePacket(elements);
//  new_receive_message(datatmp);
}

/**************************************************************************
 *                                                                        *
 * Name:       shutdown()                                                 *
 * Purpose:    sends a shutdown packet to the router before shutdown      *
 * Parameters: none                                                       *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
shutdown()
{
  find_object(DRIVER)->message("IMUD3 daemon shutting down.\n");
  sendPacket(toMUDMode( ({"shutdown",
                          5,
                          "Wyrd Dreams",
                          0,
                          router_list[0][0],
                          0,
                          (7*24*60*60)+1 }) ) );
  save_object("/kernel/data/imud3d.dat");
}

/**************************************************************************
 *                                                                        *
 * Name:       decodePacket()                                             *
 * Purpose:    decodes an IMUD3 packet into a mixed array                 *
 * Parameters: string data  - the IMUD3 packet as received                *
 * Returns:    mixed *ret   - a mixed array of data from the packet       *
 *                                                                        *
 *************************************************************************/
mixed *decodePacket(string data)
{
  mixed *ret;
  object ob;
  int i;

  if(data[0..1] != "({" || data[strlen(data)-2 .. strlen(data)-1] != "})")
  {
    error("Not an array\n");
    return 0;
  }

  /************************************************************************
   *                                                                      *
   * This is the bit that converts the packet.  All it does is write to a *
   * file in /kernel/data called imud3d.c and creates an object with our  *
   * received packet as a global mixed *.  We then call do_it() in this   *
   * object which returns the array.  We finish off by destructing the    *
   * object.                                                              *
   * The people I have discussed this method with regard it as 'cheating' *
   * but the way I see it, there's no point wasting time and effort       *
   * creating a function to do what the driver already does for us.  I'll *
   * let you decide which side you're on.                                 *
   *                                                                      *
   ***********************************************************************/
  for(i=strlen(data)-1; i; i--)
  {
    if(data[i-1..i] == "})" || data[i-1..i] == "])" )
    {
      if(data[i-2] == ',')
      {
        data=data[0..i-3] + data[i-1..];
        i--;
      }
    }
  }
  remove_file("/kernel/data/imud3d.c");
  write_file("/kernel/data/imud3d.c", 
             "mixed *ret;\n\n" + 
             "mixed *do_it()\n" +
             "{\n" +
             "  ret="+data+";\n\n" +
             "  return ret;\n" +
             "}");
  ob = compile_object("/kernel/data/imud3d");
  if(!ob)
  {
    error("Temporary object find failure.");
    return 0;
  }
  ret = ob->do_it();
  return ret;
}

/**************************************************************************
 *                                                                        *
 * Name:       handlePacket()                                             *
 * Purpose:    handles an IMUD3 packet                                    *
 * Parameters: mixed *packet - the array containing the packet data       *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
void handlePacket(mixed *packet)
{
  string *muds;
  int i;

  switch(packet[0])
  {
    case "startup-reply": //    if(sizeof(packet) != 8)
                          //      return;
                              find_object(DRIVER)->message(
                                       "Startup-reply packet received.\n");
                              password    = packet[7];
                              router_list = packet[6];
                              break;
  case "channel-m":   //        if(sizeof(packet) != 9)
                      //          return;
                            tellAllUsers(packet[6] + " " + 
                                                         packet[7] + "@" + 
                                                        packet[2] + ": " + 
                                                         packet[8] + "\n");
                            break;
  case "channel-e":   //        if(sizeof(packet) != 9)
                      //          return;
                            tellAllUsers(packet[6] + " " + 
                                                         packet[7] + "@" + 
                                                        packet[2] + " " + 
                                                         packet[8] + "\n");
                           break;
 case "error":             find_object(DRIVER)->message("Error packet:\n"+
                                                         packet[2] + ", " +
                                                         packet[4] + ", " +
                                                         packet[5] + ", " +
                                                         packet[6] + ", " +
                                                         packet[7] + 
                                                                    ",\n");
                                break;
  case "tell":               tellAllUsers(packet[6] + "@" + packet[2] + 
                                                         " tells " +
                                                         packet[5] + ": " +
                                                         packet[7] + "\n");
                             break;
  case "mudlist":	     find_object(DRIVER)->message("MUDlist received.\n");
                             if(packet[6] > mudlist_id)
                             {
                               mudlist_id = packet[6];
                               muds = map_indices(packet[7]);
                               for(i=0; i < sizeof(muds); i++)
                               {
                                 mudlist[muds[i]] = packet[7][muds[i]];
                               }
                             }
                             break;
         default:            find_object(DRIVER)->message("Packet not recognized: " + packet[0] + "\n");
  }
}

/**************************************************************************
 *                                                                        *
 * Name:       new_receive_message()                                      *
 * Purpose:    called when data arrives from the router                   *
 *             this will be replacing the receive_message function above  *
 *             when I'm sure it's working OK                              *
 * Parameters: string data - the data packet sent by the router           *
 * Returns:    none                                                       *
 *                                                                        *
 *************************************************************************/
void new_receive_message(string data)
{
  mixed *elements;
  string *newPackets;
  int packet_size, i, j;

  inBuffer += data;
  newPackets = explode(inBuffer, "})\000");

  for(i=0; i<sizeof(newPackets); i++)
  {
    packet_size = 0;
    data = newPackets[i] + "})";
    write_file("/imud3.log", data + "\n\n");
    newPackets -= ({ newPackets[i] });
    // extract the packet size
    for(j=0; j<4; j++)
    {
      packet_size <<= 8;
      packet_size += (int)data[j];
    }
    // and remove it from the front of the packet
    data = data[4..strlen(data)-1];
    // discount the trailing zero from the packet length
    packet_size--;

    if(strlen(data) != packet_size)
    {
      write_file("/imud3.log", "Bad IMUD3 data packet.  Reported size: " + packet_size + " Actual size: " + strlen(data) + ".\n" + data + "\n\n\n");
    return;
    }
  }
}
