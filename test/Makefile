#
# Stub for possible future test facility.
#
# Copyright 2017 Alexander Kulak.
# This file is part of alttab program.
#
# alttab is free software: you can redistribute it and/or modify
# 	it under the terms of the GNU General Public License as published by
# 	the Free Software Foundation, either version 3 of the License, or
# 	(at your option) any later version.
#
# alttab is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with alttab.  If not, see <http://www.gnu.org/licenses/>.
#

FLAGS=-lpng -lX11 -g -O0 -Wall 

all: pngtest

pngtest: pngtest.o ../src/pngd.o
	gcc $(FLAGS) -o pngtest pngtest.o ../src/pngd.o

pngtest.o: pngtest.c
	gcc $(FLAGS) -c pngtest.c

../src/pngd.o: ../src/pngd.c ../src/pngd.h
	gcc $(FLAGS) -o ../src/pngd.o -c ../src/pngd.c

