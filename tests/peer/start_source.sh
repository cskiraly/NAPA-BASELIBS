#!/bin/bash

echo ""
echo "PLEASE START java -cp Chunker.jar chunker.Chunker -dechunkize http://0.0.0.0:<port>/NapaTest/chunk <MCADDR> on your computer, where" \
"<port> is an available tcp port (e.g. 9901) and <MCADDR> is a multicast address to send the received stream to (e.g.  239.1.100.1:6000)"

echo ""
echo "Press Enter when ready"

read

SOURCE_STREAM="udp://239.1.1.1:5000"
reposerver=`cat source.cfg | grep repository -A 1|grep server|cut -d= -f2|tr -d \"|tr -d " "`

# clean the repository 
./repository-clean.sh $reposerver

# start multicast streaming for the chunkizer
vlc -L --sout $SOURCE_STREAM -I none --quiet ../../ul/starwars.avi &

./peer source.cfg

# start the chunkizer/dechunkizer
# it receives the multicast stream (239.1.1.1:5000) and 
# chunkizes it to the source's injector (http://127.0.0.1:1515/NapaTest/chunk) interface
java -cp Chunker.jar chunker.Chunker -chunkize `echo $SOURCE_STREAM| cut -d / -f3`  http://127.0.0.1:1515/NapaTest/chunk >/dev/null 2>/dev/null &




