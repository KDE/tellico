<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!--
   ================================================================
   Bookcase summary XSLT file

   $Id: summary.xsl,v 1.3 2002/08/26 19:22:30 robby Exp $

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
 <html>
 <head>
  <title>Bookcase Summary</title>
 </head>
 <body>
 <xsl:apply-templates select="collection"/>
 </body>
 </html>
</xsl:template>

<xsl:template match="collection">
 <h1>
 <xsl:value-of select="@title"/>
 </h1>
 <hr/>
 <p>
 <xsl:text>The collection has </xsl:text>
 <xsl:value-of select="count(book)"/>
 <xsl:text> books.</xsl:text>
 </p>
 <p>
 <xsl:for-each select="//book[generate-id(.)=generate-id(key('authors', author)[1])]">
  <xsl:sort select="author"/>
  <xsl:for-each select="key('authors', author)">
   <xsl:sort select="title"/>
   <xsl:if test="position() = 1">
    <strong>
    <xsl:value-of select="author"/>
    </strong>
    <br/>
   </xsl:if>
   <xsl:text>- </xsl:text>
   <xsl:value-of select="title"/>
   <br/>
  </xsl:for-each>
 </xsl:for-each> 
 <xsl:text>.</xsl:text>
 </p>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
