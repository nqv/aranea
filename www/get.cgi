#!/bin/sh
echo "Content-type: text/html"
echo ""
echo '<html>
<head>
  <title>Aranea CGI test</title>
</head>
<body>
  <p>Aranea CGI test</p>
  <div>
    <form action="post.cgi" method="POST">
      <label>Input 1 <input name="input1" type="text" value="Value 1" /></label><br />
      <label>Input 2 <select name="input2">
        <option value="option1">Option 1</option>
        <option value="option2">Option 2</option>
        <option value="option3">Option 3</option>
      </select></label><br />
      <label>Input 3 <textarea name="input3">Value 3</textarea></label><br />
      <input type="submit" value="Submit" />
    </form>
  </div>'
echo '<pre>'
echo "Request method: $REQUEST_METHOD"
echo "Request uri: $REQUEST_URI"
echo "Query string: $QUERY_STRING"
echo "Content type: $CONTENT_TYPE"
echo "Content length: $CONTENT_LENGTH"
echo '</pre>'
echo '</body>
</html>'

exit 0;
