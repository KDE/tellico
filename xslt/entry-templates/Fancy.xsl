<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - fancy template for viewing entry data

   Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>

   The drop-shadow effect is based on the "A List Apart" method
   at http://www.alistapart.com/articles/cssdropshadows/

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html"
            indent="yes"
            doctype-system="about:legacy-compat"
            encoding="utf-8"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="font"/> <!-- font family -->
<xsl:param name="fontsize"/> <!-- font size -->
<xsl:param name="fgcolor"/> <!-- foreground color -->
<xsl:param name="bgcolor"/> <!-- background color -->
<xsl:param name="color1"/> <!-- highlighted text color -->
<xsl:param name="color2"/> <!-- highlighted background color -->
<xsl:param name="linkcolor"/> <!-- link color -->

<xsl:param name="collection-file"/> <!-- might have a link to parent collection -->

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="fieldsByCat" match="tc:field" use="@category"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>
<xsl:key name="loansByEntry" match="tc:loan" use="@entryRef"/>

<xsl:variable name="image-width" select="'150'"/>
<xsl:variable name="image-height" select="'200'"/>
<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- all the categories -->
<xsl:variable name="categories" select="/tc:tellico/tc:collection/tc:fields/tc:field[generate-id(.)=generate-id(key('fieldsByCat',@category)[1])]/@category"/>
<!-- layout changes depending on whether there are images or not -->
<xsl:variable name="num-images" select="count(tc:tellico/tc:collection/tc:entry[1]/*[key('fieldsByName',local-name(.))/@type = 10])"/>

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
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <style>
  body {
    margin: 0px;
    padding: 0px;
    font-family: "<xsl:value-of select="$font"/>";
    font-size: <xsl:value-of select="$fontsize"/>pt;
    color: <xsl:value-of select="$fgcolor"/>;
    background-color: <xsl:value-of select="$bgcolor"/>;
    background-image: url(<xsl:value-of select="concat($imgdir, 'gradient_bg.png')"/>);
    background-repeat: repeat;
  }
  h1 {
    margin: 0px;
    padding: 4px 0px 4px 0px;
    font-size: 1.8em;
    color: <xsl:value-of select="$color1"/>;
    background-color: <xsl:value-of select="$color2"/>;
    background-image: url(<xsl:value-of select="concat($imgdir, 'gradient_header.png')"/>);
    background-repeat: repeat-x;
    border-bottom: 1px outset black;
    text-align: center;
  }
  <xsl:if test="$num-images &gt; 0">
  div#images {
    margin: 10px 5px 0px 5px;
    float: right;
    min-width: <xsl:value-of select="$image-width"/>px; /* min-width instead of width, stylesheet actually scales image */
  }
  </xsl:if>
  div.img-shadow {
    float: left;
    background: url(<xsl:value-of select="concat($datadir,'shadowAlpha.png')"/>) no-repeat bottom right;
    margin: 6px 0 10px 10px;
    clear: left;
  }
  div.img-shadow img {
    display: block;
    position: relative;
    border: 1px solid #a9a9a9;
    margin: -6px 6px 6px -6px;
  }
  h3 {
    text-align: center;
    font-size: 1.1em;
  }
  div#id {
    position: absolute;
    right: 2px;
    top: 2px;
    text-align: right;
    color: <xsl:value-of select="$color1"/>;
    background-color: <xsl:value-of select="$color2"/>;
  }
  div#content {
    padding-left: 1%;
    padding-right: 1%;
  }
  div.category {
    padding: 0px 0px 0px 4px;
    margin: 8px;
    border: 1px solid <xsl:value-of select="$fgcolor"/>;
    text-align: center;
  /* if background is grey, text has to be black */
    color: #000;
    background-color: #ccc;
    min-height: 1em;
    overflow: hidden;
  }
  h2 {
    color: <xsl:value-of select="$color1"/>;
    background-color: <xsl:value-of select="$color2"/>;
    background-image: url(<xsl:value-of select="concat($imgdir, 'gradient_header.png')"/>);
    background-repeat: repeat-x;
    border-bottom: 1px outset;
    border-color: <xsl:value-of select="$fgcolor"/>;
    padding: 4px 8px 4px 0px;
    margin: 0px 0px 2px -4px; /* -4px to match .category padding */
    font-size: 1.0em;
    top: -1px;
    font-style: normal;
    text-align: center;
  }
  table {
    border-collapse: collapse;
    border-spacing: 0px;
    max-width: 100%;
  }
  tr.table-columns {
    font-style: italic;
  }
  th.fieldName {
    font-weight: bolder;
    text-align: left;
    padding: 0px 4px 0px 2px;
    white-space: nowrap;
    vertical-align: top;
  }
  td.fieldValue {
    text-align: left;
    padding: 0px 10px 0px 2px;
    vertical-align: top;
    width: 90%; /* nowrap is set on the fieldName column, so just want enough width to take the rest */
  }
  td.column1 {
    font-weight: bold;
    text-align: left;
    padding: 0px 2px 0px 2px;
/*    white-space: nowrap;*/
  }
  td.column2 {
    font-style: italic;
    text-align: left;
    padding: 0px 10px 0px 10px;
  }
  p {
    margin: 2px 10px 2px 0;
    padding: 0px;
    text-align: justify;
    font-size: 90%;
  }
  ul {
    text-align: left;
    margin: 0px 0px 0px 0px;
    padding-left: 20px;
  }
  img {
    border: 0px;
  }
  p.navigation {
    font-weight: bold;
    text-align: center;
    clear: both;
  }
  a {
    color: <xsl:value-of select="$linkcolor"/>;
  }
  </style>
  <title>
   <xsl:value-of select="tc:collection[1]/tc:entry[1]//tc:title[1]"/>
   <xsl:text>&#xa0;&#8211; </xsl:text>
   <xsl:value-of select="tc:collection[1]/@title"/>
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

 <xsl:if test="tc:id">
  <div id="id">
   <xsl:value-of select="tc:id"/>
  </div>
 </xsl:if>

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

 <!-- all the images are in a div, aligned to the right side and floated -->
 <!-- only want this div if there are any images in the entry -->
 <xsl:if test="$num-images &gt; 0">
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
          <xsl:call-template name="image-link">
           <xsl:with-param name="image" select="key('imagesById', $image)"/>
           <xsl:with-param name="dir" select="$imgdir"/>
          </xsl:call-template>
         </xsl:otherwise>
        </xsl:choose>
       </xsl:attribute>
       <img alt="" style="vertical-align:bottom;">
        <xsl:attribute name="src">
         <xsl:call-template name="image-link">
          <xsl:with-param name="image" select="key('imagesById', $image)"/>
          <xsl:with-param name="dir" select="$imgdir"/>
         </xsl:call-template>
        </xsl:attribute>
        <!-- limit to maximum width of 150 and height of 200 -->
        <xsl:call-template name="image-size">
         <xsl:with-param name="limit-width" select="$image-width"/>
         <xsl:with-param name="limit-height" select="$image-height"/>
         <xsl:with-param name="image" select="key('imagesById', $image)"/>
        </xsl:call-template>
       </img>
      </a>
     </div>
     <br/> <!-- needed since the img-shadow block floats -->
    </xsl:if>
   </xsl:for-each>
  </div>
 </xsl:if>

 <!-- all the data is in the content block -->
 <div id="content">
  <!-- now for all the rest of the data -->
  <!-- iterate over the categories, but skip paragraphs and images -->
  <xsl:for-each select="$categories[key('fieldsByCat',.)[1]/@type != 2 and key('fieldsByCat',.)[1]/@type != 10]">
   <xsl:call-template name="output-category">
    <xsl:with-param name="entry" select="$entry"/>
    <xsl:with-param name="category" select="."/>
   </xsl:call-template>
  </xsl:for-each>

  <!-- now do paragraphs -->
  <xsl:for-each select="$categories[key('fieldsByCat',.)[1]/@type = 2]">
   <xsl:call-template name="output-category">
    <xsl:with-param name="entry" select="$entry"/>
    <xsl:with-param name="category" select="."/>
   </xsl:call-template>
  </xsl:for-each>

  <xsl:for-each select="key('loansByEntry', tc:id)">
   <div class="container">
    <div class="category">
     <h2><i18n>Loan</i18n></h2>
     <table>
      <tbody>
       <tr>
        <th class="fieldName"><i18n>Borrower</i18n></th>
        <td class="fieldValue">
         <xsl:value-of select="../@name"/>
        </td>
       </tr>
       <tr>
        <th class="fieldName"><i18n>Loan Date</i18n></th>
        <td class="fieldValue">
         <xsl:value-of select="@loanDate"/>
        </td>
       </tr>
       <tr>
        <th class="fieldName"><i18n>Due Date</i18n></th>
        <td class="fieldValue">
         <xsl:value-of select="@dueDate"/>
        </td>
       </tr>
       <tr>
        <th class="fieldName"><i18n>Note</i18n></th>
        <td class="fieldValue">
         <xsl:value-of select="."/>
        </td>
       </tr>
      </tbody>
     </table>
    </div>
   </div>
  </xsl:for-each>
 </div>

</xsl:template>

<xsl:template name="output-category">
 <xsl:param name="entry"/>
 <xsl:param name="category"/>

 <xsl:variable name="fields" select="key('fieldsByCat', $category)"/>
 <xsl:variable name="first-type" select="$fields[1]/@type"/>

 <xsl:variable name="n" select="count($entry//*[key('fieldsByName',local-name(.))/@category=$category and
                                                key('fieldsByName',local-name(.))/@name != 'id' and
                                                key('fieldsByName',local-name(.))/@name != 'title' and
                                                key('fieldsByName',local-name(.))/@name != 'cdate' and
                                                key('fieldsByName',local-name(.))/@name != 'mdate'])"/>
 <!-- only output if there are fields in this category
      also, special case, don't output empty paragraphs -->
 <xsl:if test="$n &gt; 0 and (not($first-type = 2) or $entry/*[local-name(.) = $fields[1]/@name])">
  <div class="container">
   <xsl:if test="$num-images = 0 and $first-type != 2">
    <xsl:attribute name="style">
     <!-- two columns of divs -->
     <xsl:text>width: 50%; display: block; float: left;</xsl:text>
    </xsl:attribute>
   </xsl:if>
   <xsl:if test="$num-images = 0 and $first-type = 2">
    <xsl:attribute name="style">
     <xsl:text>width: 100%; display: block; float: left;</xsl:text>
    </xsl:attribute>
   </xsl:if>
   <div class="category">

   <h2>
    <xsl:value-of select="$category"/>
   </h2>
   <!-- ok, big xsl:choose loop for field type -->
   <xsl:choose>

    <!-- paragraphs are field type 2 -->
    <xsl:when test="$first-type = 2">
     <p>
      <!-- disabling the output escaping allows html -->
      <xsl:value-of select="$entry/*[local-name(.) = $fields[1]/@name]" disable-output-escaping="yes"/>
     </p>
    </xsl:when>

    <!-- tables are field type 8 -->
    <!-- ok to put category name inside div instead of table here -->
    <xsl:when test="$first-type = 8">
     <!-- look at number of columns -->
     <xsl:choose>
      <xsl:when test="$fields[1]/tc:prop[@name = 'columns'] &gt; 1">
       <table>
        <xsl:if test="$fields[1]/tc:prop[@name = 'column1']">
         <thead>
          <tr class="table-columns">
           <th>
            <xsl:value-of select="$fields[1]/tc:prop[@name = 'column1']"/>
            <xsl:text>&#160;</xsl:text>
           </th>
           <th>
            <xsl:value-of select="$fields[1]/tc:prop[@name = 'column2']"/>
            <xsl:text>&#160;</xsl:text>
           </th>
           <xsl:call-template name="columnTitle">
            <xsl:with-param name="index" select="3"/>
            <xsl:with-param name="max" select="$fields[1]/tc:prop[@name = 'columns']"/>
            <xsl:with-param name="elem" select="'th'"/>
            <xsl:with-param name="field" select="$fields[1]"/>
           </xsl:call-template>
          </tr>
         </thead>
        </xsl:if>
        <tbody>
         <xsl:for-each select="$entry//*[local-name(.) = $fields[1]/@name]">
          <tr>
           <xsl:for-each select="tc:column">
            <xsl:choose>
             <xsl:when test="position() = 1">
              <td class="column1">
               <xsl:value-of select="."/>
               <xsl:text>&#160;</xsl:text>
              </td>
             </xsl:when>
             <xsl:otherwise>
              <td class="column2">
               <!-- special-case the tv episode table -->
               <xsl:if test="$fields[1]/@name = 'episode'">
                <xsl:attribute name="style">text-align:center</xsl:attribute>
               </xsl:if>
               <xsl:value-of select="."/>
               <xsl:text>&#160;</xsl:text>
              </td>
             </xsl:otherwise>
            </xsl:choose>
           </xsl:for-each>
          </tr>
         </xsl:for-each>
        </tbody>
       </table>
      </xsl:when>
      <xsl:otherwise>
       <ul>
        <xsl:for-each select="$entry//*[local-name(.) = $fields[1]/@name]">
         <li>
          <xsl:value-of select="."/>
         </li>
        </xsl:for-each>
       </ul>
      </xsl:otherwise>
     </xsl:choose>
    </xsl:when>

    <xsl:otherwise>
     <table>
      <tbody>
       <!-- already used title, so skip it -->
       <!-- don't show id or internal dates either -->
       <xsl:for-each select="$fields[@name != 'title' and @name != 'id' and @name != 'cdate' and @name != 'mdate']">
        <tr>
         <th class="fieldName">
          <xsl:value-of select="@title"/>
          <xsl:text>:</xsl:text>
         </th>
         <td class="fieldValue">
          <xsl:call-template name="simple-field-value">
           <xsl:with-param name="entry" select="$entry"/>
           <xsl:with-param name="field" select="@name"/>
          </xsl:call-template>
         </td>
        </tr>
       </xsl:for-each>
      </tbody>
     </table>

    </xsl:otherwise>
   </xsl:choose>

  </div>
  </div>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
