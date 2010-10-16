#!/bin/sh

gcc -Wall -g main.c -o aqdbg `gtk-config --cflags` `gtk-config --libs`
