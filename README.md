WebPal
======

Webpal is a web-based rewrite of the Palantir data viewer.

Webpal uses rudimentary bindings to [GetData](http://getdata.sourceforge.net) 
(see bindings.cc) for viewing the current state of binary data stored in a 
dirfile. A Node.js server uses a higher-level module to be notified of updates
to watched fields, and streams the updates with socket.io to clients.

Setup
=====

Build
-----

The build the low-level bindings run
    $ node-waf configure build

If you have a dirfile, try playing with bindings_test.js
    $ vi bindings_test.js
    (Update the path_to_dirfile and field_name variables)
    $ node bindings_test.js
