<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!--
   ================================================================
   Bookcase XSLT file - sort by title

   $Id: bookcase-by-title.xsl,v 1.1 2002/08/26 19:22:30 robby Exp $

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
  <title><xsl:value-of select="@title"/> - sorted by title</title>
  <style type="text/css">
  body {
        background: #999999;
        margin: 0px;
        font-family: Verdana, Arial, sans-serif;
        color: black;
  }
  #headerblock {
        background: rgb(0,0,51) none repeat scroll 0%;
        padding-top: 10px;
        padding-bottom: 10px;
        margin-bottom: 5px;
  }
  .title {
        padding: 4px;
        line-height: 18px;
        font-family: Lucida Console, Verdana, Arial, sans-serif;
        color: white;
        font-size: 24px;
        border-top: 1px solid black;
        border-bottom: 1px solid black;
        margin: 0px;
        background: rgb(51,51,102) none repeat scroll 0%;
  }
  .subtitle {
        margin-left:10px;
        font-family: Lucida Console, Verdana, Arial, sans-serif;
        font-size: 12px;
  }
  .author {
        margin-bottom: 2px;
        border-top: 1px solid black;
        border-bottom: 1px solid black;
        background: #eeeeee;
        text-align: left;
        font-family: Verdana, Arial, sans-serif;
        color: black;
        font-size: 14px;
        font-weight: bold;
  }
  .pots {
        border: 2px solid #aaaaaa;
        margin-left: 15px;
        margin-bottom: 5px;
        margin-right: 15px;
        font-family: Arial, sans-serif;
        color: black;
        font-size: 12px;
  }
  .books {
        background: rgb(204,204,204);
        padding-left: 4px;
        /*border: 2px solid #aaaaaa;*/
        margin-left: 15px;
        margin-bottom: 5px;
        margin-right: 15px;
        font-family: Arial, sans-serif;
        color: black;
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
    <xsl:value-of select="@title"/><br/>
     <span class="subtitle">sorted by title</span>
   </div>
  </div>

  <xsl:for-each select="book/title">
   <xsl:value-of select="."/><br/>
  </xsl:for-each> 
 </body>
</html>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
