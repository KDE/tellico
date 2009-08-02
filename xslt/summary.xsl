<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ================================================================
   Tellico summary XSLT file

   Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with XML data files
   from the Tellico application, which can be found at http://tellico-project.org
   ================================================================
-->

<xsl:output method="html"
            indent="yes"
            doctype-public="-//W3C//DTD HTML 4.01//EN"
            doctype-system="http://www.w3.org/TR/html4/strict.dtd"
            encoding="utf-8"/>

<xsl:key name="authors" match="tc:entry" use=".//tc:author"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <html>
 <head>
  <title>Tellico Summary</title>
 </head>
 <body>
 <xsl:apply-templates select="tc:collection"/>
 </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <h1>
  <xsl:value-of select="@title"/>
 </h1>
 <hr/>
 <p>
  <xsl:text>The collection has </xsl:text>
  <xsl:value-of select="count(tc:entry)"/>
  <xsl:text> entries.</xsl:text>
 </p>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
