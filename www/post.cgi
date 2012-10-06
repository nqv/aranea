#!/bin/bash

echo "Content-type: text/html"
echo ""
echo "Request method: $REQUEST_METHOD"
echo "Request uri: $REQUEST_URI"
echo "Query string: $QUERY_STRING"
echo "Content type: $CONTENT_TYPE"
echo "Content length: $CONTENT_LENGTH"

if [ -z $CONTENT_LENGTH = "" ]; then
	echo "Content is empty"
else
	echo "Content:"
	read -n $CONTENT_LENGTH LINE
	echo $LINE
fi
