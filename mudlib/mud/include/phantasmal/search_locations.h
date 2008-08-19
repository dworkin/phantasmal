/* search_locations.h

   These constants are used by the SYSTEM_USER::find_objects() and
   SYSTEM_USER::find_first_objects() calls to
   determine what objects are visible to the viewer.  By supplying
   a list of locations, the caller determines where, and in what
   order, the returned objects can be found.
*/

/* This looks for objects contained in the current room.  It also
   looks inside anything it reasonably can, such as open
   containers or the room's details. */
#define LOC_CURRENT_ROOM                           1

/* This looks only for objects contained in the current room -- no details,
   no inventory, nothing in containers, nothing else. */
#define LOC_IMMEDIATE_CURRENT_ROOM                 2

/* This looks for details of the current room and their details. */
#define LOC_DETAIL_CURRENT_ROOM                    3

/* This looks for objects contained in the viewer, objects in details
   of the viewer and objects in open containers within the viewer. */
#define LOC_INVENTORY                              4

/* This looks for objects immediately contained by the viewer -- nothing
   in containers, nothing in body details. */
#define LOC_IMMEDIATE_INVENTORY                    5

/* This looks for objects which are details of the viewer */
#define LOC_BODY                                   6

/* This looks for objects immediately contained by the exits */
#define LOC_CURRENT_EXITS                          7
