#!/bin/sh
# SPDX-License-Identifier: WTFPL
# やりたい放題！
# 免責条項： 本果の保証は一切ありません。
# Copyright 2022 rtldg <rtldg@protonmail.com>

for f in $(find maps/ -name '*.nav' -size 205c -type f); do rm "$f"; done
for f in $(find maps/ -name '*.nav' -size 0c -type f); do rm "$f"; done
echo -n -e \\xCE\\xFA\\xED\\xFE\\x10\\x00\\x00\\x00\\x01\\x00\\x00\\x00\\x58\\xF6\\x01\\x00\\x01\\x00\\x00\\x01\\x01\\x00\\x00\\x00\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x80\\xED\\xC3\\x00\\x00\\x48\\x42\\xFF\\x1F\\x00\\x42\\x00\\x00\\x48\\xC2\\x00\\x80\\xED\\x43\\xFF\\x1F\\x00\\x42\\xFF\\x1F\\x00\\x42\\xFF\\x1F\\x00\\x42\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x04\\x00\\x00\\x00\\x00\\x00\\x40\\xE7\\xC3\\x00\\x00\\x7A\\x42\\xFF\\x1F\\x00\\x42\\x01\\x01\\x00\\x00\\x00\\x00\\x00\\x7A\\xC2\\x00\\x00\\x7A\\x42\\xFF\\x1F\\x00\\x42\\x01\\x02\\x00\\x00\\x00\\x00\\x00\\x7A\\xC2\\x00\\x40\\xE7\\x43\\xFF\\x1F\\x00\\x42\\x01\\x03\\x00\\x00\\x00\\x00\\x40\\xE7\\xC3\\x00\\x40\\xE7\\x43\\xFF\\x1F\\x00\\x42\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\xF0\\x42\\x00\\x00\\xF0\\x42\\x00\\x00\\x80\\x3F\\x00\\x00\\x80\\x3F\\x00\\x00\\x80\\x3F\\x00\\x00\\x80\\x3F\\x01\\x00\\x00\\x00\\x01\\x00\\x00\\x00\\x02\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00 > maps/hardlink.nav
for f in $(find maps/ -name '*.bsp' -type f); do ln maps/hardlink.nav "${f%.*}.nav"; done
