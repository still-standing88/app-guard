#!/bin/bash

echo "Building AppGuard documentation..."

read -p "Install documentation requirements? (y/n): " choice
if [[ $choice =~ ^[Yy]$ ]]; then
    echo "Installing requirements..."
    pip install -r requirements.txt
else
    echo "Skipped requirements installation"
fi

echo
echo "Running Doxygen to generate C++ API documentation..."
doxygen Doxyfile

echo
echo "Building Sphinx documentation..."
cd source
sphinx-build -b html . _build

echo
echo "Documentation built successfully!"
echo "Open _build/index.html to view the documentation."