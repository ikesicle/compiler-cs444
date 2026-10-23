#!/bin/bash
make clean
git log > git.log
zip -r joosc.zip . -x "build/*" -x "docs/*" -x ".git/*" -x ".cache/*" -x "*.zip" -x "tests/*_marmoset/*" -x "tests/output/*"
rm git.log