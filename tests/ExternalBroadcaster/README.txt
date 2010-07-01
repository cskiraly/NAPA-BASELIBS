Repository-based Broadcasting
=============================

This simple demo application implements a "Broadcasting Server", which 
 - injects a video stream,
 - "chunkizes" it,
 - retrieves a peer list periodically from the Repository
 - broadcasts the stream to all the peers on the list

Usage
=====

1. Start a vlc process with a video file and instruct it to stream to your
computer using UDP streaming.
For example, on Unix, to stream to the localhost (port 5556) use:

vlc -L --sout udp://127.0.0.1:5556 -I none --quiet starwars.avi 

2. Make sure a repository server is available. Either start a
ModularRepository instance or use the public server at
repository.napa-wine.eu:9832

2. Start ONE source node:
  a) create a local source.cfg based on source.cfg.template
  b) carefully review and edit source.cfg
  c) start the source by:
  ./peer source.cfg

3. Start 1..n "client" peers:
  a) create a peer.cfg based on peer.cfg.template
  b) carefully review and edit peer.cfg
  c) (optionally) direct the "playout stream" to your PC's IP address
     start a VLC GUI on your PC and open network stream udp://0.0.0.0:port
     where the port is the same as specified in the "playout stream"
  d) start the peer by 
  ./peer peer.cfg

Important advices:
 - if you use multiple peers on the same hosts, avoid port conflicts, and
   configure a different port for each instance ("network" section of 
   the config file)
   The config file's port setting can be overridden from the command line,
   see ./peer -h for help
-  your source and "client" peers must use the same channel name ("som" 
   section of the config file). Please don't use the name "MyChannel".

