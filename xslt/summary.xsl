<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:bc ="http://periapsis.org/bookcase/"
                version="1.0">

<!--
   ================================================================
   Bookcase summary XSLT file

   $Id: summary.xsl 279 2003-11-09 02:20:40Z robby $

   Copyright (c) 2003 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with XML data files
   from the 'bookcase' application, which can be found at:
   http://www.periapsis.org/bookcase/
   ================================================================
-->

<xsl:output method="html" version="xhtml" indent="yes"/>

<xsl:strip-space elements="*"/>

<xsl:key name="authors" match="bc:book" use="bc:author"/>

<xsl:template match="/">
 <xsl:apply-templates select="bc:bookcase"/>
</xsl:template>

<xsl:template match="bc:bookcase">
 <html>
 <head>
  <title>Bookcase Summary</title>
 </head>
 <body>
 <xsl:apply-templates select="bc:collection"/>
 </body>
 </html>
</xsl:template>

<xsl:template match="bc:collection">
 <h1>
  <xsl:value-of select="@title"/>
 </h1>
 <hr/>
 <p>
  <xsl:text>The collection has </xsl:text>
  <xsl:value-of select="count(bc:entry)"/>
  <xsl:text> entries.</xsl:text>
 </p>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
