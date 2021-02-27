<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - classic template for viewing entry data

   Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>

   Known Issues:
   o Dependent titles have no value in the entry element.
   o Bool and URL fields are assumed to never have multiple values.

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html"
            indent="yes"
            doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
            doctype-system="http://www.w3.org/TR/html4/loose.dtd"
            encoding="utf-8"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="font"/> <!-- font family -->
<xsl:param name="fontsize"/> <!-- font size -->
<xsl:param name="fgcolor"/> <!-- foreground color -->
<xsl:param name="bgcolor"/> <!-- background color -->
<xsl:param name="color1"/> <!-- highlighted text color -->
<xsl:param name="color2"/> <!-- highlighted background color -->

<xsl:param name="collection-file"/> <!-- might have a link to parent collection -->

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="fieldsByCat" match="tc:field" use="@category"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- all the categories -->
<xsl:variable name="categories" select="/tc:tellico/tc:collection/tc:fields/tc:field[generate-id(.)=generate-id(key('fieldsByCat',@category)[1])]/@category"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<!-- The default layout is pretty boring, but catches every field value in
     the entry. The title is in the top H1 element. -->
<xsl:template match="tc:tellico">
 <!-- This stylesheet is designed for Tellico document syntax version 12 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'12'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <html>
  <head>
  <meta nameE="viewport" content="width=device-width, initial-scale=1"/>
  <style type="text/css">
   body {
        margin: 4px;
        padding: 0px;
        font-family: "<xsl:value-of select="$font"/>";
        font-size: <xsl:value-of select="$fontsize"/>pt;
        color: <xsl:value-of select="$fgcolor"/>;
        background-color: <xsl:value-of select="$bgcolor"/>;
   }
   h1.title {
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
        border: 0px;
   }
   table.category {
        margin-bottom: 1em;
	float: left;
   }
   tr.category {
        font-weight: bold;
        font-size: 1.2em;
        color: <xsl:value-of select="$color1"/>;
        background-color: <xsl:value-of select="$color2"/>;
        text-align: center;
   }
   th {
        font-weight: bold;
        text-align: left;
        color: #fff;
        background-color: #666;
        padding-left: 3px;
        padding-right: 3px;
   }
   td {
        padding: 0 .2em;
   }
   p {
        margin-top: 0px;
        text-align: justify;
   }
   img {
        border: 0px solid;
   }
   p.navigation {
        clear: both;
        font-weight: bold;
        text-align: center;
        clear: both;
   }
  </style>
  <title>
   <xsl:value-of select="tc:collection/tc:entry[1]//tc:title[1]"/>
   <xsl:text>&#xa0;&#8211; </xsl:text>
   <xsl:value-of select="tc:collection/@title"/>
  </title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection[1]"/>
   <xsl:if test="$collection-file">
    <p class="navigation">
     <a href="{$collection-file}">&lt;&lt; <xsl:value-of select="tc:collection/@title"/></a>
    </p>
   </xsl:if>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <xsl:apply-templates select="tc:entry[1]"/>
</xsl:template>

<xsl:template match="tc:entry">
 <xsl:variable name="entry" select="."/>

 <!-- first, show the title -->
 <xsl:if test=".//tc:title">
  <h1>
   <xsl:for-each select=".//tc:title">
    <xsl:value-of select="."/>
    <xsl:if test="position() &lt; last()">
     <br/>
    </xsl:if>
   </xsl:for-each>
  </h1>
 </xsl:if>

 <!-- put the general category and all images in top table, one cell for each -->
 <xsl:variable name="cat1" select="key('fieldsByName','title')/@category"/>
 <table width="100%">
  <tr>
   <td valign="top">
    <!-- show the general group, or more accurately, the title's group -->
    <table class="category" width="100%">
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
   <xsl:variable name="images" select="../tc:fields/tc:field[@type=10]"/>
   <xsl:if test="count($images) &gt; 0">
    <!-- now, show all the images in the entry, type 10 -->
    <xsl:for-each select="$images">

     <!-- images will never be multiple, so no need to check for that -->
     <!-- find the value of the image field in the entry -->
     <xsl:variable name="image" select="$entry/*[local-name(.) = current()/@name]"/>
     <!-- check if the value is not empty -->
     <xsl:if test="$image">
      <td valign="top">
       <table class="category">
        <tr class="category">
         <td>
          <xsl:value-of select="current()/@title"/>
         </td>
        </tr>
        <tr>
         <td>
          <a>
           <xsl:attribute name="href">
            <xsl:choose>
             <!-- Amazon license requires the image to be linked to the amazon website -->
             <xsl:when test="$entry/tc:amazon">
              <xsl:value-of select="$entry/tc:amazon"/>
             </xsl:when>
             <xsl:otherwise>
              <xsl:call-template name="image-link">
               <xsl:with-param name="image" select="key('imagesById', $image)"/>
               <xsl:with-param name="dir" select="$imgdir"/>
              </xsl:call-template>
             </xsl:otherwise>
            </xsl:choose>
           </xsl:attribute>
           <img alt="">
            <xsl:attribute name="src">
             <xsl:call-template name="image-link">
              <xsl:with-param name="image" select="key('imagesById', $image)"/>
              <xsl:with-param name="dir" select="$imgdir"/>
             </xsl:call-template>
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
  <xsl:if test="key('fieldsByCat', .)[@name != 'id' and @name != 'cdate' and @name != 'mdate']">
  <table width="50%" class="category">
   <tr class="category">
    <td colspan="2">
     <xsl:value-of select="."/>
    </td>
   </tr>
   <xsl:for-each select="key('fieldsByCat', .)[@name != 'id' and @name != 'cdate' and @name != 'mdate']">
    <tr>
     <xsl:choose>
      <!-- paragraphs -->
      <xsl:when test="@type = 2">
       <td>
        <p>
         <xsl:value-of select="$entry/*[local-name(.)=current()/@name]" disable-output-escaping="yes"/>
        </p>
       </td>
      </xsl:when>

      <!-- tables are field type 8 -->
      <!-- ok to put category name inside div instead of table here -->
      <xsl:when test="@type = 8">
       <td>
        <!-- look at number of columns -->
        <xsl:choose>
         <xsl:when test="tc:prop[@name = 'columns'] &gt; 1">
          <table width="100%">
           <xsl:if test="tc:prop[@name = 'column1']">
            <thead>
             <tr>
              <th width="50%">
               <xsl:value-of select="tc:prop[@name = 'column1']"/>
              </th>
              <th width="50%">
               <xsl:value-of select="tc:prop[@name = 'column2']"/>
              </th>
              <xsl:call-template name="columnTitle">
               <xsl:with-param name="index" select="3"/>
               <xsl:with-param name="max" select="tc:prop[@name = 'columns']"/>
               <xsl:with-param name="elem" select="'th'"/>
               <xsl:with-param name="field" select="."/>
              </xsl:call-template>
             </tr>
            </thead>
           </xsl:if>
           <tbody>
            <xsl:for-each select="$entry//*[local-name(.) = current()/@name]">
             <tr>
              <xsl:for-each select="tc:column">
               <td width="{floor(100 div count(../tc:column))}%">
                <xsl:value-of select="."/>
                <xsl:text>&#160;</xsl:text>
               </td>
              </xsl:for-each>
             </tr>
            </xsl:for-each>
           </tbody>
          </table>
         </xsl:when>
         <xsl:otherwise>
          <ul>
           <xsl:for-each select="$entry//*[local-name(.) = current()/@name]">
            <li>
             <xsl:value-of select="."/>
            </li>
           </xsl:for-each>
          </ul>
         </xsl:otherwise>
        </xsl:choose>
       </td>
      </xsl:when>

      <!-- everything else -->
      <xsl:otherwise>
       <th>
        <xsl:value-of select="@title"/>
       </th>
       <td>
        <xsl:call-template name="simple-field-value">
         <xsl:with-param name="entry" select="$entry"/>
         <xsl:with-param name="field" select="@name"/>
        </xsl:call-template>
       </td>
      </xsl:otherwise>
     </xsl:choose>
    </tr>
   </xsl:for-each>
  </table>
  </xsl:if>
 </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
