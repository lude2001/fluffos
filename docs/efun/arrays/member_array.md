---
layout: doc
title: arrays / member_array
---
# member_array

### NAME

    member_array()  -  returns  index of an occurence of a given item in an
    array or string

### SYNOPSIS

    int member_array( mixed item, mixed * | string arr,
                      void | int start, void | int flags );

### DESCRIPTION

    Returns the index of the first occurence of 'item' in array  'arr',  or
    the  first  occurence  at  or after 'start'.  If the item is not found,
    then -1 is returned.

    Note, if the second argument is a string, the first parameter must be an int
    representing the character you are looking for in the provided string.

    The optional 'flags' argument is a bit field:

    1 - 'item' is a string prefix.
    2 - search backwards from the end of the array and return the last match.
    4 - 'item' is a function predicate called with each element.

### EXAMPLE

    member_array( "red", ({ "red", "blue", "red", "green", "red", }) ) ;
    // 0
    
    member_array( "red", ({ "red", "blue", "red", "green", "red", }) , 1) ;
    // 2

    member_array( "blue", ({ "red", "blue", "red", "green", "red", }) , 3) ;
    // -1

    member_array( "purple", ({ "red", "blue", "red", "green", "red", }) ) ;
    // -1

    member_array('F', "Drink the FluffOS Kool-Aid!") ;
    // 10

    member_array('Z', "Drink the FluffOS Kool-Aid!") ;
    // -1

    member_array( (: $1 > 10 :), ({ 1, 5, 42, 77 }), 0, 4 ) ;
    // 2



