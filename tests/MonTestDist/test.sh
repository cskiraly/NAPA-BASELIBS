#/bin/bash -f
REPO="130.192.86.30:9832"
#REPO="repository.napa-wine.eu:9832"
CHANNEL="MonTestDist"
PORTS="5000 5001 5002 5003 5004 5005"
CYCLE_TIME="600"

for PORT in $PORTS
do
	rm -rf $PORT
	mkdir $PORT
	cd $PORT
	xterm -e "catchsegv ../MonTestDist -p $PORT -c $CHANNEL -r $REPO -C $CYCLE_TIME  &> log.txt;read" &
	cd ..
done
