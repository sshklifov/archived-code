#!/bin/bash

cd "$HOME/Downloads/glfw-3.2.1/src"

for file in $(ls); do
    if $(cat $file | grep -q "glfwGetTime"); then
        echo "Found it! ($file)"
    fi
done
