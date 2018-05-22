#!/bin/bash
git -C axoloti pull
git -C axoloti submodule update --init --recursive
git -C axoloti-contrib pull
git -C axoloti-factory pull
