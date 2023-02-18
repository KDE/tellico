<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Image Grid

   Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at https://tellico-project.org

   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html"
            indent="yes"
            doctype-system="about:legacy-compat"
            encoding="utf-8"/>

<xsl:param name="filename"/>
<xsl:param name="cdate"/>

<!-- Sort using user's preferred language -->
<xsl:param name="lang"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data files are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="basedir"/> <!-- relative dir for template -->

<xsl:key name="imagesById" match="tc:image" use="@id"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <html>
  <head>
   <meta name="viewport" content="width=device-width, initial-scale=1"/>
   <style type="text/css">
   body {
        font-family: sans-serif;
        background-color: #fff;
        color: #000;
   }
   #header-left {
        margin-top: 0;
        float: left;
        font-size: 80%;
        font-style: italic;
   }
   #header-right {
        margin-top: 0;
        float: right;
        font-size: 80%;
        font-style: italic;
   }
   h1.colltitle {
        margin: 0px;
        padding-bottom: 5px;
        font-size: 2em;
        text-align: center;
    }
    div.grid {
        display: grid;
        grid-gap: 5px;
        grid-template-columns: repeat(6, 1fr);
    }
    div.container {
        position: relative;
    }
    div.container img {
         width: 100%;
         height: 100%;
         object-fit: cover;
    }
    div.container p {
         background-color: rgba(0,0,0,0.5);
         color: #fff;
         display: flex;
         align-items: center;
         justify-content: center;
         position: absolute;
         width: 100%;
         height: 100%;
         top:0;
         left: 0;
         opacity: 0;
         transition: opacity .5s ease;
         font-size: 80%;
         font-weight: bold;
         margin: 0;
    }
    div.container:hover p {
         opacity: 1;
    }
   </style>
   <title>
    <xsl:value-of select="tc:collection/@title"/>
   </title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <p id="header-left"><xsl:value-of select="$filename"/></p>
 <p id="header-right"><xsl:value-of select="$cdate"/></p>
 <h1 class="colltitle">
  <xsl:value-of select="@title"/>
 </h1>

 <div class="grid">
  <!-- find first image field -->
  <xsl:variable name="image-field" select="tc:fields/tc:field[@type=10][1]/@name"/>
  
  <xsl:for-each select="tc:entry">
   <xsl:sort lang="$lang" select=".//tc:title[1]"/>
   <xsl:variable name="entry" select="."/>
   
   <div class="container">
    <xsl:variable name="id" select="./*[local-name() = $image-field]"/>
    <xsl:if test="$id">
     <img alt="{./tc:title}">
      <xsl:attribute name="src">
       <xsl:call-template name="image-link">
        <xsl:with-param name="image" select="key('imagesById', $id)"/>
        <xsl:with-param name="dir" select="$imgdir"/>
       </xsl:call-template>
      </xsl:attribute>
      <xsl:call-template name="image-size">
       <xsl:with-param name="image" select="key('imagesById', $id)"/>
      </xsl:call-template>
     </img>
    </xsl:if>
    <p>
     <xsl:value-of select=".//tc:title[1]"/>
    </p>
   </div>
  </xsl:for-each>
 </div>
</xsl:template>

</xsl:stylesheet>
