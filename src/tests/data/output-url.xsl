<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

 <xsl:output method="html"
            indent="yes"
            encoding="utf-8"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <html>
  <head>
   <title>Test</title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection[1]"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <xsl:apply-templates select="tc:entry[1]"/>
</xsl:template>

<xsl:template match="tc:entry">
 <h1>
  <a href="{tc:url}">
   <xsl:value-of select="tc:url"/>
  </a>
 </h1>
</xsl:template>

</xsl:stylesheet>
