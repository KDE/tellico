<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ================================================================
   Tellico XSLT file - sort by author

   Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with XML data files
   from the 'tellico' application, which can be found at http://tellico-project.org
   ================================================================
-->

<xsl:output method="html"
            indent="yes"
            doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
            doctype-system="http://www.w3.org/TR/html4/loose.dtd"
            encoding="utf-8"/>

<!-- Sort using user's preferred language -->
<xsl:param name="lang"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <xsl:if test="@syntaxVersion &lt; '11'">
  <xsl:message>
   <xsl:text>This stylesheet was designed for Tellico DTD version </xsl:text>
   <xsl:value-of select="'7'"/>
   <xsl:text>or earlier, &#xa;but the input data file is version </xsl:text>
   <xsl:value-of select="@syntaxVersion"/>
   <xsl:text>. There might be some &#xa;problems with the output.</xsl:text>
  </xsl:message>
 </xsl:if>

 <html>
  <head>
   <title>My Book Collections - sorted by title</title>
   <style type="text/css">
   body {
         background: #999;
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
   <xsl:apply-templates select="tc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <div id="headerblock">
  <div class="title">
   <xsl:value-of select="@title"/>
   <xsl:text> </xsl:text>
   <span class="subtitle">sorted by title</span>
  </div>
 </div>

 <div class="books">
  <ol>
   <xsl:for-each select="/tc:tellico/tc:collection/tc:entry">
    <xsl:sort lang="$lang" select="tc:title"/>
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </ol>
 </div>
</xsl:template>

<xsl:template match="tc:entry">
 <li>
  <xsl:value-of select="tc:title"/><br/>
 </li>
</xsl:template>

</xsl:stylesheet>
