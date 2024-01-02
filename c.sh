#!/bin/bash
gcc mapeditor.c shared_f.c -o "edit" -lncursesw
gcc cman.c shared_f.c -o "cman" -lncursesw
