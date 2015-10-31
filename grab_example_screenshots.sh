#!/bin/bash

screenshot() {
    example=$1
    sleeptime=$2
    title=$3

    if [ -z "$title" ] ; then
        title=${example}.py
    fi

    cd $ROOT/examples
    python ${example}.py &
    pid=$!
    sleep $sleeptime
    import -window "$title" $ROOT/doc/_static/${example}.png
    kill $pid
}


ROOT=$(pwd)

screenshot pygame_bubble 7 'Lepton pygame BlitRenderer example'
screenshot pygame_fill 3 'Lepton pygame FillRenderer example'
screenshot bouncy 5
screenshot fire 1
screenshot fireworks 3
screenshot flyby 2
screenshot generate 2
screenshot letters 10
screenshot logo 1
screenshot magnet 5
screenshot smoke 10
screenshot splode2d 1
screenshot splode 1
screenshot tunnel 1
screenshot vortex 5


cd $ROOT
