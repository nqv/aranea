#!/bin/sh
echo 'Content-type: text/html'
echo ''
echo '<html>
<head>
  <title>Aranea CGI test</title>
</head>
<body>
  <h1>Index of ./</h1>
  <pre>'

if [ -z "$DOCUMENT_ROOT" ]; then
  # print self directory
  SCRIPTPATH=`readlink -f $0`
  DOCUMENT_ROOT=`dirname $SCRIPTPATH`
fi

find "$DOCUMENT_ROOT" -mindepth 1 -maxdepth 1  -printf \
  '<a href="%f">%f</a>    %s    %Tx %Tr\n'

echo '  </pre>
</body>
</html>'
exit 0
