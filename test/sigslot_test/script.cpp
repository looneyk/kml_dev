//------------------------------------------------------------------------------
// script.cpp
//------------------------------------------------------------------------------

example button press opens door

door
    state

button
    automatically calls build in on_button_pressed callback if set
    on_button_pressed callback toggles state of door

where should on_button_pressed reside?
who should set on_button_pressed callback?

script file
button_id, on_button_pressed (event_id), code (find door by id, toggle state)

