<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!--
   ================================================================
   Bookcase XSLT file - sort by author

   $Id: bookcase-by-author.xsl,v 1.4 2002/10/12 06:01:03 robby Exp $

   Copyright (c) 2002 Robby Stephenson

   This XSLT stylesheet is designed to be used with XML data files
   from the 'bookcase' application, which can be found at:
   http://periapsis.org/bookcase/
   ================================================================
-->

<xsl:output method="html" version="xhtml" indent="yes"/>
<xsl:key name="authors" match="book" use="author"/>

<xsl:template match="/">
 <xsl:apply-templates select="bookcase"/>
</xsl:template>

<xsl:template match="bookcase">
 <xsl:apply-templates select="collection"/>
</xsl:template>

<xsl:template match="collection">
 <html>
 <head>
  <title><xsl:value-of select="@title"/> - sorted by author</title>
  <style type="text/css">
  body {
        background: #999999;
        margin: 0px;
        font-family: Verdana, Arial, sans-serif;
        color: black;
  }
  #headerblock {
        padding-top: 10px;
        padding-bottom: 10px;
        margin-bottom: 5px;
  }
  .title {
        padding: 4px;
        line-height: 18px;
        font-size: 24px;
        border-top: 1px solid black;
        border-bottom: 1px solid black;
        margin: 0px;
  }
  .subtitle {
        margin-left: 10px;
        font-size: 12px;
  }
  .author {
        margin-right: 3px;
        margin-bottom: 2px;
        background: #eee;
        font-size: 14px;
        font-weight: bold;
  }
  .books {
        background: rgb(204,204,204);
        padding-left: 4px;
        margin-left: 15px;
        margin-bottom: 5px;
        margin-right: 15px;
        font-size: 12px;
  }
  ul {
        margin: 0px;
        padding: 0px;
  }
  </style>
 </head>
 <body>
  <div id="headerblock">
   <div class="title">
    <xsl:value-of select="@title"/>
     <span class="subtitle">sorted by author</span>
   </div>
  </div>

  <xsl:for-each select="//book[generate-id(.)=generate-id(key('authors', author)[1])]">
   <xsl:sort select="author"/>
   <div class="author">
    <xsl:value-of select="author"/>
   </div>
   <div class="books">
    <ul>
     <xsl:for-each select="key('authors', author)">
      <xsl:sort select="title"/>
      <li>
       <xsl:value-of select="title"/>
      </li>
     </xsl:for-each>
    </ul>
   </div>
  </xsl:for-each> 
 </body>
</html>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
