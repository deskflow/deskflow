This is the directory for news snippets used by towncrier: https://github.com/twisted/towncrier

When changing code in a way that's visible to an end user please make a new file in this directory.
It will be removed and integrated into release notes document upon a release of a new version of
Barrier.

towncrier has a few standard types of news fragments, signified by the file extension. These are:

.feature: Signifying a new feature.
.bugfix: Signifying a bug fix.
.doc: Signifying a documentation improvement.
.removal: Signifying a deprecation or removal of public API.
