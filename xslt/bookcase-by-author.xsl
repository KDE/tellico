<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
                xmlns:bc ="http://periapsis.org/bookcase/"
                version="1.0">

<!--
   ================================================================
   Bookcase XSLT file - sort by author

   $Id: bookcase-by-author.xsl 394 2004-01-24 23:17:42Z robby $

   Copyright (C) 2003, 2004 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with XML data files
   from the 'bookcase' application, which can be found at:
   http://www.periapsis.org/bookcase/
   ================================================================
-->

<xsl:output method="html" version="xhtml" encoding="UTF-8" indent="yes"/>

<xsl:strip-space elements="*"/>

<xsl:param name="version"/>

<xsl:key name="books" match="bc:book" use=".//bc:author"/>
<xsl:key name="authors" match="bc:author" use="."/>

<!-- more efficient to specify complete XPath like this than to use //bc:author -->
<xsl:variable name="unique-authors" select="/bc:bookcase/bc:collection/bc:entry/bc:authors/bc:author[generate-id(.)=generate-id(key('authors', .)[1])]"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="bc:bookcase"/>
</xsl:template>

<xsl:template match="bc:bookcase">
 <!-- This stylesheet is designed for Bookcase document syntax version 5 -->
 <xsl:if test="@syntaxVersion != '5'">
  <xsl:message>
   <xsl:text>This stylesheet was designed for Bookcase DTD version </xsl:text>
   <xsl:value-of select="'5'"/>
   <xsl:text>, &#xa;but the input data file is version </xsl:text>
   <xsl:value-of select="@syntaxVersion"/>
   <xsl:text>. There might be some &#xa;problems with the output.</xsl:text>
  </xsl:message>
 </xsl:if>

 <html>
  <head>
   <title>My Book Collections - sorted by author</title>
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
   <xsl:apply-templates select="bc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="bc:collection">
 <div id="headerblock">
  <div class="title">
   <xsl:value-of select="@title"/>
   <xsl:text> </xsl:text>
   <span class="subtitle">sorted by author</span>
  </div>
 </div>

 <!-- first output any with no author -->
 <xsl:variable name="no-author" select="/bc:bookcase/bc:collection/bc:entry[count(bc:authors/bc:author) = 0"/>
 <xsl:if test="count($no-author) &gt; 0">
  <div class="author">
   <xsl:text>(Empty)</xsl:text>
  </div>
  <div class="books">
   <ul>
    <xsl:for-each select="$no-author"> 
     <xsl:sort select="bc:title"/>
     <xsl:apply-templates select="."/>
    </xsl:for-each>
   </ul>
  </div>
 </xsl:if> 

 <xsl:for-each select="$unique-authors">
  <xsl:sort select="."/>
  <div class="author">
   <xsl:value-of select="."/>
  </div>
  <div class="books">
   <ul>
    <xsl:for-each select="key('books', .)"> 
     <xsl:sort select="bc:title"/>
<!-- or sort by series and number -->
<!-- <xsl:sort select="bc:series"/>
     <xsl:sort select="bc:series_num"/> -->
     <xsl:apply-templates select="."/>
    </xsl:for-each>
   </ul>
  </div>
 </xsl:for-each> 
</xsl:template>

<xsl:template match="bc:entry">
 <li>
  <xsl:value-of select="bc:title"/>
 </li>
</xsl:template>

</xsl:stylesheet>
