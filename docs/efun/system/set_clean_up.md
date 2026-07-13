---
layout: doc
title: system / set_clean_up
---
# set_clean_up

### NAME

    set_clean_up - schedule when an object should next be queried for clean_up

### SYNOPSIS

    varargs void set_clean_up( object ob, int time );

### DESCRIPTION

    Schedules the driver's next clean_up() query on 'ob' for 'time' seconds
    from now. If 'time' is omitted, any explicit deadline is cancelled and the
    object returns to the normal idle-time clean_up rule.

    In both forms the object is marked for clean_up consideration if it defines
    a clean_up() apply. Objects without a clean_up() apply are never queried.

    The clean_up sweep runs periodically, so 'time' is a lower bound rather than
    an exact firing time. If the 'time to clean up' config setting is 0,
    clean_up is globally disabled.

### SEE ALSO

    clean_up(4), request_clean_up(3), set_reset(3)
