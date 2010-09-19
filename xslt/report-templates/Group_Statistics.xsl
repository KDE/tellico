<?xml version="1.0"?>
<!-- WARNING: Tellico uses tc as the internal namespace declaration, and it must be identical here!! -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                xmlns:dyn="http://exslt.org/dynamic"
                extension-element-prefixes="exsl dyn"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Collection Summary Report

   Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html"
            indent="yes"
            doctype-public="-//W3C//DTD HTML 4.01//EN"
            doctype-system="http://www.w3.org/TR/html4/strict.dtd"
            encoding="utf-8"/>

<xsl:param name="filename"/>
<xsl:param name="cdate"/>

<xsl:variable name="apos">'</xsl:variable>

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <html>
  <head>
   <script language="javascript" type="text/javascript" src="jquery.min.js"></script>
   <script language="javascript" type="text/javascript" src="jquery.flot.js"></script>
   <script language="javascript" type="text/javascript" src="jquery.flot.pie.js"></script>
   <script language="javascript" type="text/javascript">
$(function () {

  var data = { items: [] };

 <xsl:variable name="coll" select="tc:collection"/>
 <!-- grouping flag is second bit from right -->
 <xsl:for-each select="tc:collection/tc:fields/tc:field[@type != 4 and boolean(floor(@flags div 2) mod 2)]">
  <xsl:call-template name="data-items">
   <xsl:with-param name="coll" select="$coll"/>
   <xsl:with-param name="field" select="."/>
  </xsl:call-template>
 </xsl:for-each>

 for(i=0; i &lt; data.items.length; i++) {
   $.plot($("#graph"+i), data.items[i],
    {
      series: {
        pie: {
          show: true,
          combine: {
            color: '#999',
            threshold: 0.04,
            label: '<i18n>Other</i18n>'
          },
          label: {
            show: true,
            radius: 0.97
          }
        }
      },
      legend: {
        show: false
      },
      colors: ["#cb4b4b", "#4da74d", "#3300cc", "#ff9900", "#9440ed"],
    });
  }

});
</script>

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
   div.graph {
        width: 500px;
        height: 400px;
   }
   .pieLabel {
       font-weight: bold;
       text-shadow: 1px 1px 1px #CCC;
    }
   </style>
   <title>
    <xsl:value-of select="tc:collection/@title"/>
   </title>
  </head>
  <body id="body">
   <xsl:apply-templates select="tc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <p id="header-left"><xsl:value-of select="$filename"/></p>
 <p id="header-right"><xsl:value-of select="$cdate"/></p>
 <h1 class="colltitle">
  <xsl:value-of select="@title"/>
  <i18n>: Group Summary</i18n>
 </h1>

 <xsl:for-each select="tc:fields/tc:field[@type != 4 and boolean(floor(@flags div 2) mod 2)]">
  <h2><xsl:value-of select="@title"/></h2>
  <div class="graph" id="{concat('graph',position()-1)}"></div>
 </xsl:for-each>

</xsl:template>

<xsl:template name="data-items">
 <xsl:param name="coll"/>
 <xsl:param name="field"/>

 <xsl:variable name="fieldname" select="$field/@name"/>
 <xsl:variable name="value-expr">
  <xsl:for-each select="$coll/tc:entry">
   <xsl:variable name="entry" select="."/>
   <xsl:choose>
    <!-- tables -->
    <xsl:when test="$field/@type=8">
     <xsl:for-each select="./*[local-name() = concat($fieldname,'s')]/*">
      <value>
       <xsl:value-of select="tc:column[1]"/>
      </value>
     </xsl:for-each>
    </xsl:when>
    <xsl:when test="boolean(floor(key('fieldsByName', $fieldname)/@flags mod 2))">
     <xsl:for-each select="./*[local-name() = concat($fieldname,'s')]/*">
      <value>
       <xsl:value-of select="."/>
      </value>
     </xsl:for-each>
    </xsl:when>
    <xsl:otherwise>
     <xsl:for-each select="./*[local-name() = $fieldname]">
     <value>
      <xsl:call-template name="simple-field-value">
       <xsl:with-param name="entry" select="$entry"/>
       <xsl:with-param name="field" select="$fieldname"/>
      </xsl:call-template>
     </value>
     </xsl:for-each>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:for-each>
 </xsl:variable>

 <xsl:variable name="values" select="exsl:node-set($value-expr)/value"/>
 <xsl:variable name="listing">
  <xsl:for-each select="$values[not(. = preceding-sibling::*)]">
   <xsl:variable name="c" select="count($values[. = current()])"/>
   <xsl:if test="$c &gt; 1 and string-length(.) &gt; 0">
    <group>
     <xsl:attribute name="name">
      <xsl:value-of select="."/>
     </xsl:attribute>
     <xsl:attribute name="count">
      <xsl:value-of select="$c"/>
     </xsl:attribute>
    </group>
   </xsl:if>
  </xsl:for-each>
 </xsl:variable>

 <xsl:variable name="groups" select="exsl:node-set($listing)/group"/>

 <xsl:if test="count($groups) &gt; 2">
  <xsl:text>var item = [</xsl:text>
  <xsl:for-each select="$groups">
   <xsl:sort select="@count" data-type="number" order="descending" />
   <xsl:text>{ label:'</xsl:text>
   <xsl:value-of select="translate(@name, $apos, '')"/>
   <xsl:text>', data: </xsl:text>
   <xsl:value-of select="@count"/>
   <xsl:text>},</xsl:text>
  </xsl:for-each>
  <xsl:text>];
  data.items.push(item);
  </xsl:text>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
