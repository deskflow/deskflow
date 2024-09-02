#!/bin/bash

# List all tags matching the pattern "v1.*"
for oldtag in $(git tag -l "v1.*"); do
    # Remove the "v" prefix
    newtag=${oldtag#v}
    
    git tag $newtag $oldtag
    git push origin $newtag

    git push --delete origin $oldtag
    
    echo "Renamed tag $oldtag to $newtag"
done
