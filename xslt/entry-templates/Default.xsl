<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:bc="http://periapsis.org/bookcase/"
                exclude-result-prefixes="bc"
                version="1.0">

<!--
   ===================================================================
   Bookcase XSLT file - default template for viewing entry data

   $Id: Default.xsl 791 2004-08-23 00:30:27Z robby $

   Copyright (C) 2003, 2004 Robby Stephenson - robby@periapsis.org

   Known Issues:
   o Dependent titles have no value in the entry element.
   o Bool and URL fields are assumed to never have multiple values.

   This XSLT stylesheet is designed to be used with the 'Bookcase'
   application, which can be found at http://www.periapsis.org/bookcase/
   ===================================================================
-->

<!-- import common templates -->
<!-- location depe
nds on being installed correctly -->
<xsl:import href="../bookcase-common.xsl"/>

<xsl:output method="html"/>

<xsl:param name="datadir"/> <!-- dir where Bookcase data are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="font"/> <!-- default KDE font family -->
<xsl:param name="fgcolor"/> <!-- default KDE foreground color -->
<xsl:param name="bgcolor"/> <!-- default KDE background color -->
<xsl:param name="color1"/> <!-- default KDE highlighted text color -->
<xsl:param name="color2"/> <!-- default KDE highlighted background color -->

<xsl:param name="collection-file"/> <!-- might have a link to parent collection -->

<xsl:strip-space elements="*"/>

<xsl:key name="fieldsByName" match="bc:field" use="@name"/>
<xsl:key name="fieldsByCat" match="bc:field" use="@category"/>
<xsl:key name="imagesById" match="bc:image" use="@id"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- all the categories -->
<xsl:variable name="categories" select="/bc:bookcase/bc:collection/bc:fields/bc:field[generate-id(.)=generate-id(key('fieldsByCat',@category)[1])]/@category"/>

<xsl:template match="/">
 <xsl:apply-templates select="bc:bookcase"/>
</xsl:template>

<!-- The default layout is pretty boring, but catches every field value in
     the entry. The title is in the top H1 element. -->
<xsl:template match="bc:bookcase">
 <!-- This stylesheet is designed for Bookcase document syntax version 6 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'6'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <html>
  <head>
  <style type="text/css">
   body {
        margin: 4px;
        padding: 0px;
        font-family: "<xsl:value-of select="$font"/>";
        color: #000;
        background-color: #fff;
   }
   h1.title {
        color: <xsl:value-of select="$color2"/>;
        background-color: <xsl:value-of select="$color1"/>;
        font-size: 1.8em;
        padding: 0px;
        padding-top: 2px;
        padding-bottom: 2px;
        margin: 0px;
        text-align: center;
        font-weight: bold;
   }
   img {
        padding-right: 10px;
   }
   table.category {
        margin-bottom: 10px;
   }
   tr.category {
        font-weight: bold;
        font-size: 1.2em;
        color: #fff;
        background-color: #666;
        text-align: center;
   }
   th {
        font-weight: bold;
        text-align: left;
        background-color: #ccc;
        padding-left: 3px;
        padding-right: 3px;
   }
   p {
        margin-top: 0px;
   }
  </style>
  <title>
   <xsl:value-of select="bc:collection/bc:entry[1]//bc:title[1]"/>
   <xsl:text> - </xsl:text>
   <xsl:value-of select="bc:collection/@title"/>
  </title>
  </head>
  <body>
   <xsl:apply-templates select="bc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="bc:collection">
 <xsl:apply-templates select="bc:entry[1]"/>
 <xsl:if test="$collection-file">
  <hr/>
  <h4 style="text-align:center"><a href="{$collection-file}">&lt;&lt; <xsl:value-of select="@title"/></a></h4>
 </xsl:if>
</xsl:template>

<xsl:template match="bc:entry">
 <xsl:variable name="entry" select="."/>

 <!-- first, show the title -->
 <xsl:if test=".//bc:title">
  <h1 class="title">
   <xsl:value-of select=".//bc:title[1]"/>
  </h1>
 </xsl:if>

 <!-- put the general category and all images in top table, one cell for each -->
 <xsl:variable name="cat1" select="key('fieldsByName','title')/@category"/>
 <table cellspacing="0" cellpadding="0" width="100%">
  <tr>
   <td valign="top" width="100%">
    <!-- show the general group, or more accurately, the title's group -->
    <table cellspacing="1" cellpadding="0" class="category" width="100%">
     <tr class="category">
      <td colspan="2">
       <xsl:value-of select="$cat1"/>
      </td>
     </tr>
     <xsl:for-each select="key('fieldsByCat', $cat1)">
      <xsl:if test="@name!='title'">
       <tr>
        <th>
         <xsl:value-of select="@title"/>
        </th>
        <td width="75%">
         <xsl:call-template name="simple-field-value">
          <xsl:with-param name="entry" select="$entry"/>
          <xsl:with-param name="field" select="@name"/>
         </xsl:call-template>
        </td>
       </tr>
      </xsl:if>
     </xsl:for-each>
    </table>
   </td>
   <!-- now, show all the images in the entry, type 10 -->
   <xsl:variable name="images" select="../bc:fields/bc:field[@type=10]"/>
   <xsl:if test="count($images) &gt; 0">
    <!-- now, show all the images in the entry, type 10 -->
    <xsl:for-each select="$images">

     <!-- images will never be multiple, so no need to check for that -->
     <!-- find the value of the image field in the entry -->
     <xsl:variable name="image" select="$entry/*[local-name(.) = current()/@name]"/>
     <!-- check if the value is not empty -->
     <xsl:if test="$image">
      <td valign="top">
       <table cellspacing="1" cellpadding="0" class="category">
        <tr class="category">
         <td>
          <xsl:value-of select="current()/@title"/>
         </td>
        </tr>
        <tr>
         <td>
          <a>
           <xsl:attribute name="href">
            <xsl:value-of select="concat($imgdir, $image)"/>
           </xsl:attribute>
           <img>
            <xsl:attribute name="src">
             <xsl:value-of select="concat($imgdir, $image)"/>
            </xsl:attribute>
            <!-- limit to maximum width of 150 of height of 200 -->
            <xsl:call-template name="image-size">
             <xsl:with-param name="limit-width" select="150"/>
             <xsl:with-param name="limit-height" select="200"/>
             <xsl:with-param name="image" select="key('imagesById', $image)"/>
            </xsl:call-template>
           </img>
          </a>
         </td>
        </tr>
       </table>
      </td>
     </xsl:if>
    </xsl:for-each>
   </xsl:if>
  </tr>
 </table>

 <!-- write categories other than general and images -->
 <xsl:for-each select="$categories[. != $cat1 and
                       key('fieldsByCat',.)[1]/@type!=10]">
  <table cellspacing="1" cellpadding="0" width="50%" align="left" class="category">
   <tr class="category">
    <td colspan="2">
     <xsl:value-of select="."/>
    </td>
   </tr>
   <xsl:for-each select="key('fieldsByCat', .)">
    <tr>
     <xsl:choose>
      <!-- paragraphs -->
      <xsl:when test="@type=2">
       <td>
        <p>
         <xsl:value-of select="$entry/*[local-name(.)=current()/@name]" disable-output-escaping="yes"/>
        </p>
       </td>
      </xsl:when>
      <!-- single-column table -->
      <xsl:when test="@type=8">
       <td>
        <ul>
         <xsl:for-each select="$entry//*[local-name(.)=current()/@name]">
          <li>
           <xsl:value-of select="."/>
          </li>
         </xsl:for-each>
        </ul>
       </td>
      </xsl:when>
      <!-- double-column table -->
      <xsl:when test="@type=9">
       <td>
        <table width="100%" cellspacing="0" cellpadding="0">
         <xsl:for-each select="$entry//*[local-name(.)=current()/@name]">
          <tr>
           <td width="50%">
            <xsl:value-of select="bc:column[1]"/>
           </td>
           <td width="50%">
            <em>
             <xsl:value-of select="bc:column[2]"/>
            </em>
           </td>
          </tr>
         </xsl:for-each>
        </table>
       </td>
      </xsl:when>
      <!-- everything else -->
      <xsl:otherwise>
       <th>
        <xsl:value-of select="@title"/>
       </th>
       <td width="50%">
        <xsl:call-template name="simple-field-value">
         <xsl:with-param name="entry" select="$entry"/>
         <xsl:with-param name="field" select="@name"/>
        </xsl:call-template>
       </td>
      </xsl:otherwise>
     </xsl:choose>
    </tr>
   </xsl:for-each>
   <tr>
    <td>
     <br/>
    </td>
   </tr>
  </table>
  <xsl:if test="position() mod 2 = 0">
   <br clear="left"/>
  </xsl:if>
 </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
