#!/bin/sh -l

meson build
result = $(meson test -C build -v)
echo result