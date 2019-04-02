#!/bin/sh
# To be run from the ngram source directory apps/ngram/src
APPS=../..
PACKAGE=apps-ngram-1.0.0
tar -C $APPS --transform "s,^,$PACKAGE/," -czf $APPS/../$PACKAGE.tgz --exclude-vcs --exclude=sc --exclude=results ngram  misc shared
