#!/bin/sh
set -e

# Функция для установки пакетов с помощью apt-get
f() { sudo apt-get install --no-install-recommends -y "$@"; }

# Обновление списка пакетов
sudo apt-get update -y

# Установка пакетов
f libgstreamer-plugins-base1.0-0 cmake make pkg-config libx11-dev xcb libx11-xcb-dev libxkbcommon-x11-0 libxkbcommon-dev libgtk-3-dev sassc

# Установка qtbase5-dev и qtchooser
f qtbase5-dev qtchooser qtbase5-dev-tools