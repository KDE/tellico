<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                         version="1.0">

<xsl:output method="text"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="keymap"/>
</xsl:template>

<xsl:template match="keymap">
 <xsl:apply-templates select="key"/>
</xsl:template>

<xsl:template match="key">
 <xsl:text>@book { book</xsl:text>
 <xsl:value-of select="position()"/>
 <xsl:text>,</xsl:text>
 <xsl:value-of select="$endl"/>

 <xsl:text>title = {title red</xsl:text>
 <xsl:value-of select="string[1]"/>
 <xsl:value-of select="string[2]"/>
 <xsl:text>},</xsl:text>
 <xsl:value-of select="$endl"/>

 <xsl:text>author = {Robby </xsl:text>
 <xsl:value-of select="position()"/>
 <xsl:text>},</xsl:text>
 <xsl:value-of select="$endl"/>

 <xsl:text>year = 2003,</xsl:text>
 <xsl:value-of select="$endl"/>

 <xsl:text>publisher = {Baen}</xsl:text>
 <xsl:value-of select="$endl"/>

 <xsl:text>}</xsl:text>
 <xsl:value-of select="$endl"/>
</xsl:template>

</xsl:stylesheet>
