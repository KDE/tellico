#!/bin/sh
#This file outputs in a separate line each file with a .desktop syntax
#that needs to be translated but has a non .desktop extension
echo src/fetch/z3950-servers.cfg
ls -1 src/fetch/scripts/*.spec
