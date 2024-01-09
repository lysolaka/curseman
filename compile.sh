#!/bin/bash
gcc mapeditor.c shared_f.c -o "editor" -lncursesw
gcc cman.c shared_f.c -o "curseman" -lncursesw
