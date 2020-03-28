#!/bin/bash

path=../dashboard/index.html

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    xdg-open "$path"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    open "$path"
elif [[ "$OSTYPE" == "cygwin" ]]; then
    # POSIX compatibility layer and Linux environment emulation for Windows
    cygstart "$path"
elif [[ "$OSTYPE" == "msys" ]]; then
    # Lightweight shell and GNU utilities compiled for Windows (part of MinGW)
    start "$path"
elif [[ "$OSTYPE" == "win32" ]]; then
    # I'm not sure this can happen.
    start "$path"
else
    # Unknown.
    echo "Unknown OS"
fi