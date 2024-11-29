<?xml version="1.0"?>
<!-- WARNING: Tellico uses tc as the internal namespace declaration, and it must be identical here!! -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Group View Report

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
            doctype-system="about:legacy-compat"
            encoding="utf-8"/>

<xsl:param name="filename"/>
<xsl:param name="cdate"/>
<xsl:param name="basedir"/> <!-- relative dir for template -->

<!-- Sort using user's preferred language -->
<xsl:param name="lang"/>

<xsl:key name="entriesById" match="tc:entry" use="@id"/>
<xsl:key name="loansByDate" match="tc:loan" use="@dueDate"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <html>
  <head>
   <base href="{$basedir}"/>
   <meta name="viewport" content="width=device-width, initial-scale=1"/>
   <style type="text/css">
   body {
        font-family: sans-serif;
        background-color: #fff;
   }
   .header {
        display: flex;
   }
   .box {
        flex: 1;
        display: flex;
        justify-content: center;
        align-items: top;
   }
   .header-left > span {
        margin-right: auto;
        font-size: 80%;
        font-style: italic;
   }
   .header-right > span {
        margin-left: auto;
        font-size: 80%;
        font-style: italic;
   }
   .header-center > h1 {
        margin: 0px;
        padding-bottom: 5px;
   }
   table {
        margin-left: auto;
        margin-right: auto;
        margin-bottom: 30px;
   }
   td.groupName {
        margin-top: 10px;
        margin-bottom: 2px;
        padding-left: 4px;
        background: #ccc;
        font-size: 1.1em;
        font-weight: bolder;
   }
   td.fieldName {
        margin-top: 10px;
        margin-bottom: 2px;
        color: #666;
        background-color: #ccc;
        font-size: 1.1em;
        text-align: center;
        font-style: italic;
        padding-left: 4px;
        padding-right: 4px;
   }
   tr.r0 {
   }
   tr.r1 {
        background-color: #eee;
   }
   td.field {
        margin: 0px;
        padding: 0px 10px 0px 10px;
        border: 1px solid #eee;
        text-align: left;
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
 <div class="header">
  <div class="box header-left"><span><xsl:value-of select="$filename"/></span></div>
  <div class="box header-center"><h1><xsl:value-of select="@title"/></h1></div>
  <div class="box header-right"><span><xsl:value-of select="$cdate"/></span></div>
 </div>

 <table>
  <tbody>
   <!-- TODO: this would need to be fixed for multiple collections -->
   <xsl:for-each select="../tc:borrowers/tc:borrower/tc:loan[generate-id(.)=generate-id(key('loansByDate', @dueDate)[1])]">
    <xsl:sort lang="$lang" select="@dueDate"/>
    <tr>
     <td class="groupName">
      <xsl:value-of select="@dueDate"/>
     </td>
     <td class="fieldName"><i18n>Borrower</i18n></td>
     <td class="fieldName"><i18n>Loan Date</i18n></td>
     <td class="fieldName"><i18n>Note</i18n></td>
    </tr>

    <xsl:for-each select="key('loansByDate', @dueDate)">
     <tr class="r{position() mod 2}">
      <td class="field">
       <xsl:value-of select="key('entriesById', @entryRef)//tc:title"/>
      </td>
      <td class="field">
       <xsl:value-of select="../@name"/>
      </td>
      <td class="field">
       <xsl:value-of select="@loanDate"/>
      </td>
      <td class="field">
       <p><xsl:value-of select="text()"/></p>
      </td>
     </tr>
    </xsl:for-each>
   </xsl:for-each>

  </tbody>
 </table>

 <table>
  <tbody>
   <!-- TODO: this would need to be fixed for multiple collections -->
   <xsl:for-each select="../tc:borrowers/tc:borrower">
    <xsl:sort lang="$lang" select="@name"/>
    <tr>
     <td class="groupName">
      <xsl:value-of select="@name"/>
     </td>
     <td class="fieldName"><i18n>Loan Date</i18n></td>
     <td class="fieldName"><i18n>Due Date</i18n></td>
     <td class="fieldName"><i18n>Note</i18n></td>
    </tr>

    <xsl:for-each select="tc:loan">
     <tr class="r{position() mod 2}">
      <td class="field">
       <xsl:value-of select="key('entriesById', @entryRef)//tc:title"/>
      </td>
      <td class="field">
       <xsl:value-of select="@loanDate"/>
      </td>
      <td class="field">
       <xsl:value-of select="@dueDate"/>
      </td>
      <td class="field">
       <p><xsl:value-of select="text()"/></p>
      </td>
     </tr>
    </xsl:for-each>
   </xsl:for-each>

  </tbody>
 </table>

 <!--
 <h4><a href="https://tellico-project.org"><i18n>Generated by Tellico</i18n></a></h4>
-->
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
