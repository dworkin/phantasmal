#!/bin/bash

rm -rf release

make -C website
find "website" -name "*.html" |
	grep -v ".base.html" |
	grep -v ".sec.html" > tmp/list
find "website/css" >> tmp/list
find "website/DGD/external" >> tmp/list
find "website/images" >> tmp/list

mkdir -p tmp/release

for file in `cat tmp/list | egrep -v "/.svn(/|\$)"`
do
	cp -v --parents $file tmp/release
done

cd tmp/release
tar -cv website | gzip > website.tar.gz
cd ../..
mv tmp/release/website.tar.gz .
rm -rf tmp/release tmp/list
