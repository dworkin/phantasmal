#!/bin/bash

rm -rf release

make -C testsite
find "testsite" -name "*.html" |
	grep -v ".base.html" > tmp/list
find "testsite/css" >> tmp/list
find "testsite/images" >> tmp/list

mkdir -p tmp/release

for file in `cat tmp/list | egrep -v "/.svn(/|\$)"`
do
	cp -v --parents $file tmp/release
done

cd tmp/release
tar -cv testsite | gzip > testsite.tar.gz
cd ../..
mv tmp/release/testsite.tar.gz .
rm -rf tmp/release tmp/list
