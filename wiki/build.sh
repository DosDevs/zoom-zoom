#!/bin/sh

gmake clean html
sudo rm -rf /serv/Fetrovsky.org/oswiki
sudo mkdir /serv/Fetrovsky.org/oswiki
sudo cp -R build/html/. /serv/Fetrovsky.org/oswiki/

