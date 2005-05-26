<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - compact template for viewing entry data

   $Id: Compact.xsl 1185 2005-05-08 16:57:21Z robby $

   Copyright (C) 2003-2005 Robby Stephenson - robby@periapsis.org

   The drop-shadow effect is based on the "A List Apart" method
   at http://www.alistapart.com/articles/cssdropshadows/

   Known Issues:
   o Dependent titles have no value in the entry element.
   o Bool and URL fields are assumed to never have multiple values.

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="font"/> <!-- default KDE font family -->
<xsl:param name="fgcolor"/> <!-- default KDE foreground color -->
<xsl:param name="bgcolor"/> <!-- default KDE background color -->

<xsl:param name="collection-file"/> <!-- might have a link to parent collection -->

<xsl:strip-space elements="*"/>

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<!-- The default layout is pretty boring, but catches every field value in
     the entry. The title is in the top H1 element. -->
<xsl:template match="tc:tellico">
 <!-- This stylesheet is designed for Tellico document syntax version 7 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'7'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <html>
  <head>
  <style type="text/css">
  body {
    margin: 0px;
    padding: 0px;
    font-family: <xsl:value-of select="$font"/>;
    color: <xsl:value-of select="$fgcolor"/>;
    background-color: <xsl:value-of select="$bgcolor"/>;
  }
  #content {
    padding-right: 160px;
  }
  #images {
    margin: 6px 5px 0px 5px;
    float: right;
    min-width: 150px;
  }
  div.img-shadow {
    float: left;
    background: url(<xsl:value-of select="concat($datadir,'shadowAlpha.png')"/>) no-repeat bottom right;
    margin: 10px 0 0 10px;
  }
  div.img-shadow img {
    display: block;
    position: relative;
    border: 1px solid #a9a9a9;
    margin: -6px 6px 6px -6px;
  }
  table {
    border-collapse: collapse;
    border-spacing: 0px;
  }
  th.fieldName {
    font-weight: bold;
    text-align: left;
    padding: 0px 4px 0px 2px;
    white-space: nowrap;
  }
  td.fieldValue {
    text-align: left;
    padding: 0px 10px 0px 2px;
  }
  td.column1 {
    text-align: left;
    padding: 0px;
  }
  td.column2 {
    font-style: italic;
    text-align: left;
    padding: 0px 10px;
  }
  p {
    margin-top: 0px;
    text-align: left;
  }
  ul {
    margin-top: 0px;
    margin-bottom: 0px;
    padding: 0px 0px 0px 20px;
  }
  </style>
  <title>
   <xsl:value-of select="tc:collection/tc:entry[1]//tc:title[1]"/>
   <xsl:text> - </xsl:text>
   <xsl:value-of select="tc:collection/@title"/>
  </title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection[1]"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <xsl:apply-templates select="tc:entry[1]"/>
 <xsl:if test="$collection-file">
  <hr/>
  <h4 style="text-align:center"><a href="{$collection-file}">&lt;&lt; <xsl:value-of select="@title"/></a></h4>
 </xsl:if>
 <xsl:if test="tc:entry[1]/tc:amazon">
  <hr/>
  <p style="text-align:center">Data provided by <a href="{tc:entry[1]/tc:amazon}">Amazon.com</a></p>  
 </xsl:if>
</xsl:template>

<xsl:template match="tc:entry">
 <xsl:variable name="entry" select="."/>

 <!-- all the images are in a div, aligned to the right side and floated-->
 <div id="images">
  <!-- images are field type 10 -->
  <xsl:for-each select="../tc:fields/tc:field[@type=10]">

   <!-- find the value of the image field in the entry -->
   <xsl:variable name="image" select="$entry/*[local-name(.) = current()/@name]"/>
   <!-- check if the value is not empty -->
   <xsl:if test="$image">
    <div class="img-shadow">
     <a>
      <xsl:attribute name="href">
       <xsl:choose>
        <!-- Amazon license requires the image to be linked to the amazon website -->
        <xsl:when test="$entry/tc:amazon">
         <xsl:value-of select="$entry/tc:amazon"/>
        </xsl:when>
        <xsl:otherwise>
         <xsl:value-of select="concat($imgdir, $image)"/>
        </xsl:otherwise>
       </xsl:choose>
      </xsl:attribute>
      <img>
       <xsl:attribute name="src">
        <xsl:value-of select="concat($imgdir, $image)"/>
       </xsl:attribute>
       <!-- limit to maximum width of 150 and height of 200 -->
       <xsl:call-template name="image-size">
        <xsl:with-param name="limit-width" select="150"/>
        <xsl:with-param name="limit-height" select="200"/>
        <xsl:with-param name="image" select="key('imagesById', $image)"/>
       </xsl:call-template>
      </img>
     </a>
    </div>
    <br/> <!-- needed since the img-shadow block floats -->
   </xsl:if>
  </xsl:for-each>
 </div>

  <!-- all the data is in the content block -->
 <div id="content">

  <!-- now for all the rest of the data -->
  <!-- iterate over the categories, but skip images -->
  <table>
   <tbody>
    <xsl:for-each select="../tc:fields/tc:field[@type!=10]">
     <xsl:variable name="field" select="."/>
     
     <xsl:if test="$entry//*[local-name(.) = $field/@name]">
      <tr>
      <th class="fieldName" valign="top">
       <xsl:value-of select="@title"/>
       <xsl:text>:</xsl:text>
      </th>
      
      <td class="fieldValue">
       <!-- ok, big xsl:choose loop for field type -->
       <xsl:choose>
        
        <!-- paragraphs are field type 2 -->
        <xsl:when test="@type = 2">
         <p>
          <xsl:value-of select="$entry/*[local-name(.) = $field/@name]" disable-output-escaping="yes"/>
         </p>
        </xsl:when>
        
        <!-- single-column tables are field type 8 -->
        <xsl:when test="@type = 8">
         <ul>
          <xsl:for-each select="$entry//*[local-name(.) = $field/@name]">
           <li>
            <xsl:value-of select="."/>
           </li>
          </xsl:for-each>
         </ul>
        </xsl:when>
        
        <!-- double-column tables are field type 9 -->
        <xsl:when test="@type = 9">
         <table>
          <tbody>
           <xsl:for-each select="$entry//*[local-name(.) = $field/@name]">
            <tr>
             <td class="column1">
              <xsl:value-of select="tc:column[1]"/>
             </td>
             <td class="column2">
              <xsl:value-of select="tc:column[2]"/>
             </td>
            </tr>
           </xsl:for-each>
          </tbody>
         </table>
        </xsl:when>
        
        <xsl:otherwise>
         <xsl:call-template name="simple-field-value">
          <xsl:with-param name="entry" select="$entry"/>
          <xsl:with-param name="field" select="$field/@name"/>
         </xsl:call-template>
        </xsl:otherwise>
        
       </xsl:choose>
      </td>
     </tr>
    </xsl:if>
   </xsl:for-each>
  </tbody>
 </table>
 </div>
</xsl:template>

</xsl:stylesheet>
