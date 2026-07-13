---
layout: doc
title: contrib / set_os_env
---
# set_os_env

### NAME

    set_os_env - set or unset an allow-listed OS environment variable

### SYNOPSIS

    int set_os_env( string name, string value );
    int set_os_env( string name );

### DESCRIPTION

    Sets the operating-system environment variable 'name' to 'value', or unsets
    it when 'value' is omitted. Returns 1 on success and 0 on an
    operating-system level failure.

    Writes are denied unless 'name' appears in the runtime config option
    "writable os environment variables". Writable names are also readable via
    get_os_env().

### SEE ALSO

    get_os_env(3)
