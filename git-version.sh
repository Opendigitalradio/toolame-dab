#!/bin/sh
if [ -d .git ]
then
    git rev-parse --short HEAD
else
    echo "unknown"
fi

