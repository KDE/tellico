<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:mods="http://www.loc.gov/mods/v3"
                xmlns:l="uri:langs"
                exclude-result-prefixes="mods l"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing MODS files.

   Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<!-- lookup table for languages -->
<!-- according to http://www.oasis-open.org/cover/iso639-2-en971019.html -->
<!-- I just added common ones -->
<l:langs>
 <l:lang id="ara">Arabic</l:lang>
 <l:lang id="cat">Catalan</l:lang>
 <l:lang id="cze">Czech</l:lang>
 <l:lang id="dut">Dutch</l:lang>
 <l:lang id="eng">English</l:lang>
 <l:lang id="fre">French</l:lang>
 <l:lang id="ger">German</l:lang>
 <l:lang id="dut">Dutch</l:lang>
 <l:lang id="heb">Hebrew</l:lang>
 <l:lang id="gre">Greek</l:lang>
 <l:lang id="hin">Hindi</l:lang>
 <l:lang id="hun">Hungarian</l:lang>
 <l:lang id="ita">Italian</l:lang>
 <l:lang id="jpn">Japanese</l:lang>
 <l:lang id="kor">Korean</l:lang>
 <l:lang id="lat">Latin</l:lang>
 <l:lang id="lit">Lithuanian</l:lang>
 <l:lang id="nob">Norwegian Bokm&#229;l</l:lang>
 <l:lang id="nor">Norwegian</l:lang>
 <l:lang id="nno">Norwegian Nynorsk</l:lang>
 <l:lang id="pol">Polish</l:lang>
 <l:lang id="por">Portuguese</l:lang>
 <l:lang id="rus">Russian</l:lang>
 <l:lang id="slo">Slovak</l:lang>
 <l:lang id="spa">Spanish</l:lang>
 <l:lang id="swe">Swedish</l:lang>
 <l:lang id="chi">Chinese</l:lang>
</l:langs>
<xsl:key name="langs" match="l:lang" use="@id"/>
<xsl:variable name="langs-top" select="document('')/*/l:langs"/>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<!-- for now, it's a bibtex collection if it has an identifier,
     book collection if not -->
<xsl:variable name="type">
 <xsl:choose>
  <xsl:when test="count(//mods:mods[string-length(@ID)&gt;0 or mods:identifier[@type='citekey']]) &gt; 0">
   <xsl:text>bibtex</xsl:text>
  </xsl:when>
  <xsl:otherwise>
   <xsl:text>book</xsl:text>
  </xsl:otherwise>
 </xsl:choose>
</xsl:variable>

<xsl:variable name="npubs">
 <xsl:value-of select="count(//mods:originInfo[count(mods:publisher) &gt; 1 and not(ancestor::mods:extension)])"/>
</xsl:variable>

<!-- disable default behavior -->
<xsl:template match="text()|@*"></xsl:template>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="MODS Import">
   <xsl:attribute name="type">
    <xsl:choose>
     <xsl:when test="$type='bibtex'">
      <xsl:text>5</xsl:text>
     </xsl:when>
     <xsl:when test="$type='book'">
      <xsl:text>2</xsl:text>
     </xsl:when>
    </xsl:choose>
   </xsl:attribute>
   <fields>
    <field name="_default"/>
    <!-- the default collection does not have multiple publishers -->
    <xsl:if test="$npubs &gt; 1">
     <field flags="7" title="Publisher" category="Publishing" format="0" type="1" name="publisher" i18n="true">
      <prop name="bibtex">publisher</prop>
     </field>
    </xsl:if>
    <!-- default book collection does not include an abstract -->
    <xsl:if test="$type='book' and .//mods:mods/mods:abstract">
     <field flags="0" title="Abstract" format="4" type="2" name="abstract" i18n="true">
      <prop name="bibtex">abstract</prop>
     </field>
    </xsl:if>
    <!-- default book collection does not include a doi field -->
    <xsl:if test="$type='book' and .//mods:mods/mods:identifier[@type='doi']">
     <field flags="0" title="DOI" category="Publishing" description="Digital Object Identifier" format="4" type="1" name="doi" i18n="true">
      <prop name="bibtex">doi</prop>
     </field>
    </xsl:if>
    <!-- default book collection does not include an address -->
    <xsl:if test="$type='book' and .//mods:originInfo/mods:place/mods:placeTerm[@type='text']">
     <field flags="6" title="Address" category="Publishing" format="4" type="1" name="address" i18n="true">
      <prop name="bibtex">address</prop>
     </field>
    </xsl:if>
    <!-- add illustrator -->
    <xsl:if test=".//mods:mods/mods:name[@type='personal']/mods:role/mods:roleTerm[@authority='marcrelator' and @type='code'] = 'ill.'">
     <field flags="7" title="Illustrator" category="General" format="2" type="1" name="illustrator" i18n="true"/>
    </xsl:if>
    <xsl:if test=".//mods:identifier[@type='issn']">
     <field flags="0" title="ISSN#" category="Publishing" format="4" type="1" name="issn" description="ISSN#" />
    </xsl:if>
    <xsl:if test=".//mods:classification[@authority='ddc']">
     <field title="Dewey Decimal" flags="0" category="Publishing" format="4" type="1" name="dewey" i18n="true"/>
    </xsl:if>
    <xsl:if test=".//mods:classification[@authority='lcc']">
     <field title="LoC Classification" flags="0" category="Publishing" format="4" type="1" name="lcc" i18n="true"/>
    </xsl:if>
   </fields>
<!-- for now, go the route of bibliox, and assume only text records
     with an originInfo/publisher or identifier[isbn] elements are actually books -->
<!-- Changed in Tellico 1.1, don't be so strict about the text thing, not every library
     includes that in the mods output, so just check for publisher
     Changed in Tellico 1.2.9, allow anything that has typeOfResource='text'
     //mods:mods[(mods:typeOfResource='text' and -->
<!-- Changed in Tellico 3.0.1 and 2.3.13, don't read mods info in extension elements -->
   <xsl:for-each select=".//mods:mods[ (mods:typeOfResource='text' or
                                        mods:originInfo/mods:publisher or
                                        mods:identifier[@type='isbn'] or
                                        mods:identifier[@type='lccn']) and
                                       not(ancestor::mods:extension) ]">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="mods:mods">
 <entry>

  <xsl:if test="$type='bibtex'">
   <entry-type>
    <xsl:choose>
     <xsl:when test="mods:genre[@authority='xbib']='article'">
      <xsl:text>article</xsl:text>
     </xsl:when>
     <xsl:when test="mods:genre[@authority='xbib']='book'">
      <xsl:text>book</xsl:text>
     </xsl:when>
     <xsl:when test="mods:identifier[@type='isbn']">
      <xsl:text>book</xsl:text>
     </xsl:when>
     <xsl:when test="mods:genre[@authority='marc']='theses'">
      <xsl:text>phdthesis</xsl:text>
     </xsl:when>
     <xsl:when test="mods:relatedItem[@type='host']/mods:genre[@authority='marc']='periodical'">
      <xsl:text>article</xsl:text>
     </xsl:when>
     <xsl:when test="mods:relatedItem[@type='host']/mods:genre[@authority='marc']='book'">
      <xsl:text>inbook</xsl:text>
     </xsl:when>
     <xsl:when test="mods:relatedItem[@type='host']/mods:name[@type='conference']">
      <xsl:text>inproceedings</xsl:text>
     </xsl:when>
     <xsl:when test="mods:relatedItem[@type='host']">
      <xsl:text>incollection</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:text>book</xsl:text>
     </xsl:otherwise>
    </xsl:choose>
   </entry-type>

   <!-- identifier overrides @ID -->
   <xsl:for-each select="(mods:identifier[@type='citekey']|@ID)[1]">
    <bibtex-key>
     <xsl:value-of select="."/>
    </bibtex-key>
   </xsl:for-each>
  </xsl:if>

  <!-- prefer the titles without a type specified, but ensure a non-empty value -->
  <xsl:apply-templates select="(mods:titleInfo[string-length(mods:title) &gt; 0 and not(@type)] |
                                mods:titleInfo[string-length(mods:title) &gt; 0])[1]"/>

  <subtitle>
   <xsl:value-of select="mods:titleInfo/mods:subTitle"/>
  </subtitle>

  <xsl:call-template name="names">
   <xsl:with-param name="elem" select="'author'"/>
   <!-- 3/27/17 : added @usage='primary' for HathiTrustFetcher responses -->
   <xsl:with-param name="nodes" select="mods:name[mods:role/mods:roleTerm[@type='text'] = 'author' or
                                                  mods:role/mods:roleTerm[@type='text'] = 'creator' or
                                                  (@type='personal' and @usage='primary')]"/>
  </xsl:call-template>

  <xsl:call-template name="names">
   <xsl:with-param name="elem" select="'editor'"/>
   <xsl:with-param name="nodes" select="mods:name[mods:role/mods:roleTerm[@authority='marcrelator' and @type='text'] = 'editor']"/>
  </xsl:call-template>

  <xsl:call-template name="names">
   <xsl:with-param name="elem" select="'illustrator'"/>
   <xsl:with-param name="nodes" select="mods:name[mods:role/mods:roleTerm[@authority='marcrelator' and @type='code'] = 'ill.']"/>
  </xsl:call-template>

  <genres i18n="true">
   <xsl:for-each select="mods:genre">
    <genre>
     <xsl:value-of select="."/>
    </genre>
   </xsl:for-each>
  </genres>

  <xsl:choose>
   <xsl:when test="$npubs &gt; 1">
    <publishers>
     <xsl:apply-templates select="mods:originInfo/mods:publisher"/>
    </publishers>
   </xsl:when>
   <xsl:otherwise>
    <xsl:apply-templates select="mods:originInfo/mods:publisher"/>
   </xsl:otherwise>
  </xsl:choose>

  <!-- prefer the marc encoding for year -->
  <!-- force numbers...is that ok? -->
  <xsl:variable name="year-type">
   <xsl:choose>
    <xsl:when test="$type='bibtex'">
     <xsl:text>year</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:text>pub_year</xsl:text>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:variable>

  <xsl:choose>
   <xsl:when test="mods:originInfo/mods:dateIssued[@encoding='marc']">
    <xsl:element name="{$year-type}">
     <xsl:call-template name="numbers">
      <xsl:with-param name="value" select="mods:originInfo/mods:dateIssued[@encoding='marc'][1]"/>
     </xsl:call-template>
    </xsl:element>
   </xsl:when>
   <xsl:when test="mods:originInfo/mods:dateIssued">
    <xsl:element name="{$year-type}">
     <xsl:call-template name="numbers">
      <xsl:with-param name="value" select="mods:originInfo/mods:dateIssued[1]"/>
     </xsl:call-template>
    </xsl:element>
   </xsl:when>
  </xsl:choose>

  <languages i18n="true">
   <xsl:for-each select="mods:language/mods:languageTerm">
    <language>
     <xsl:apply-templates select="$langs-top">
      <xsl:with-param name="lang-id" select="text()"/>
     </xsl:apply-templates>
    </language>
   </xsl:for-each>
  </languages>

  <xsl:if test="$type='book'">
   <comments>
    <xsl:for-each select="mods:note | mods:physicalDescription/*">
     <xsl:value-of select="."/>
     <!-- add separating line, Tellico understands HTML now -->
     <xsl:text>&lt;br/&gt;&lt;br/&gt;</xsl:text>
    </xsl:for-each>
   </comments>
  </xsl:if>
  <xsl:if test="$type='bibtex'">
   <note>
    <xsl:for-each select="mods:note | mods:physicalDescription/*">
     <xsl:value-of select="."/>
     <!-- add separating line, Tellico understands HTML now -->
     <xsl:text>&lt;br/&gt;&lt;br/&gt;</xsl:text>
    </xsl:for-each>
   </note>
  </xsl:if>

  <keywords i18n="true">
   <xsl:for-each select="mods:subject/mods:topic">
    <keyword>
     <xsl:value-of select="."/>
    </keyword>
   </xsl:for-each>
  </keywords>

  <xsl:apply-templates select="mods:relatedItem[@type='host'] |
                               mods:relatedItem[@type='series'] |
                               mods:part |
                               mods:identifier |
                               mods:classification |
                               mods:subTitle |
                               mods:abstract |
                               mods:originInfo/mods:place |
                               mods:originInfo/mods:copyrightDate |
                               mods:originInfo/mods:edition |
                               mods:location/mods:url"/>

 </entry>
</xsl:template>

<xsl:template match="mods:titleInfo">
 <title>
  <xsl:value-of select="normalize-space(concat(mods:nonSort, mods:title))"/>
 </title>
</xsl:template>

<xsl:template match="mods:relatedItem">
 <xsl:variable name="elem">
  <xsl:choose>
   <xsl:when test="@type='series'">
    <xsl:text>series</xsl:text>
   </xsl:when>
   <xsl:when test="mods:genre[@authority='marc']='periodical'">
    <xsl:text>journal</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:text>booktitle</xsl:text>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>

 <xsl:element name="{$elem}">
  <xsl:value-of select="mods:titleInfo/mods:nonSort"/>
  <xsl:value-of select="mods:titleInfo/mods:title"/>
  <xsl:if test="not(mods:titleInfo) and mods:name[@type='conference']">
   <xsl:apply-templates select="mods:name[@type='conference']"/>
  </xsl:if>
 </xsl:element>

 <xsl:call-template name="names">
  <xsl:with-param name="elem" select="'editor'"/>
  <xsl:with-param name="nodes" select="mods:name[mods:role/mods:roleTerm[@authority='marcrelator' and @type='text'] = 'editor']"/>
 </xsl:call-template>

 <!-- Tellico 3.5.5, don't read publisher from relatedInfo -->
 <!--
 <xsl:choose>
  <xsl:when test="$npubs &gt; 1">
   <publishers>
    <xsl:apply-templates select="mods:originInfo/mods:publisher"/>
   </publishers>
  </xsl:when>
  <xsl:otherwise>
   <xsl:apply-templates select="mods:originInfo/mods:publisher"/>
  </xsl:otherwise>
 </xsl:choose>
-->

 <xsl:choose>
  <xsl:when test="mods:originInfo/mods:dateIssued[@encoding='marc']">
   <year>
    <xsl:call-template name="numbers">
     <xsl:with-param name="value" select="mods:originInfo/mods:dateIssued[@encoding='marc'][1]"/>
    </xsl:call-template>
   </year>
  </xsl:when>
  <xsl:when test="mods:originInfo/mods:dateIssued">
   <year>
    <xsl:call-template name="numbers">
     <xsl:with-param name="value" select="mods:originInfo/mods:dateIssued[1]"/>
    </xsl:call-template>
   </year>
  </xsl:when>
 </xsl:choose>

 <xsl:apply-templates select="mods:part |
                              mods:originInfo/mods:place |
                              mods:identifier |
                              mods:part |
                              mods:location/mods:url"/>

</xsl:template>

<xsl:template match="mods:part">
 <xsl:apply-templates select="mods:detail | mods:extent"/>
</xsl:template>

<xsl:template name="names">
 <xsl:param name="elem"/>
 <xsl:param name="nodes"/>
 <xsl:if test="$nodes">
  <xsl:element name="{concat($elem,'s')}">
   <xsl:for-each select="$nodes">
    <xsl:element name="{$elem}">
     <xsl:apply-templates select="."/>
    </xsl:element>
   </xsl:for-each>
  </xsl:element>
 </xsl:if>
</xsl:template>

<xsl:template match="mods:name">
 <!-- specific order -->
 <xsl:apply-templates select="mods:namePart[@type='given']"/>
 <xsl:apply-templates select="mods:namePart[@type='family']"/>
 <xsl:apply-templates select="mods:namePart[not(@type)]"/>
</xsl:template>

<xsl:template match="mods:namePart">
 <!-- MARC2MODS v3.7 started added a comma at end of some names -->
 <xsl:choose>
  <xsl:when test="substring(., string-length(.)) = ','">
   <xsl:value-of select="substring(., 1, string-length(.)-1)"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="."/>
  </xsl:otherwise>
 </xsl:choose>
 <!-- assume single-character name parts are initials -->
 <xsl:if test="string-length()=1">
  <xsl:text>.</xsl:text>
 </xsl:if>
  <xsl:text> </xsl:text>
</xsl:template>

<xsl:template match="mods:classification">
 <xsl:choose>
  <xsl:when test="@authority='ddc'">
   <dewey>
    <xsl:value-of select="."/>
   </dewey>
  </xsl:when>
  <xsl:when test="@authority='lcc'">
   <lcc>
    <xsl:value-of select="."/>
   </lcc>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="mods:identifier">
 <xsl:choose>
  <xsl:when test="@type='isbn'">
   <isbn>
    <xsl:value-of select="."/>
   </isbn>
  </xsl:when>
  <xsl:when test="@type='lccn'">
   <lccn>
    <xsl:value-of select="."/>
   </lccn>
  </xsl:when>
  <xsl:when test="@type='doi'">
   <doi>
    <xsl:value-of select="."/>
   </doi>
  </xsl:when>
  <xsl:when test="@type='issn'">
   <issn>
    <xsl:value-of select="."/>
   </issn>
  </xsl:when>
  <xsl:when test="@type='uri'">
   <url>
    <xsl:value-of select="."/>
   </url>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="mods:subTitle">
 <subtitle>
  <xsl:value-of select="."/>
 </subtitle>
</xsl:template>

<xsl:template match="mods:publisher">
 <publisher>
  <xsl:value-of select="."/>
 </publisher>
</xsl:template>

<xsl:template match="mods:copyrightDate">
 <cr_year>
  <xsl:call-template name="numbers">
   <xsl:with-param name="value" select="."/>
  </xsl:call-template>
 </cr_year>
</xsl:template>

<xsl:template match="mods:abstract">
 <abstract>
  <xsl:value-of select="."/>
 </abstract>
</xsl:template>

<xsl:template match="mods:edition">
 <edition i18n="true">
  <xsl:value-of select="."/>
 </edition>
</xsl:template>

<xsl:template match="mods:url">
 <url>
  <xsl:value-of select="."/>
 </url>
</xsl:template>

<xsl:template match="mods:place">
 <address>
  <xsl:value-of select="mods:placeTerm[@type='text' or not(@type)][1]"/>
 </address>
</xsl:template>

<xsl:template match="mods:detail">
 <xsl:choose>
  <xsl:when test="@type='volume'">
   <volume>
    <xsl:value-of select="mods:number"/>
   </volume>
  </xsl:when>
  <xsl:when test="@type='issue'">
   <number>
    <xsl:value-of select="mods:number"/>
   </number>
  </xsl:when>
  <xsl:when test="@type='number'">
   <number>
    <xsl:value-of select="mods:number"/>
   </number>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="mods:extent">
 <xsl:if test="@unit='page'">
  <pages>
   <xsl:choose>
    <xsl:when test="mods:start and mods:end">
     <xsl:value-of select="concat(mods:start,'-',mods:end)"/>
    </xsl:when>
    <xsl:when test="mods:list">
     <xsl:value-of select="mods:list"/>
    </xsl:when>
   </xsl:choose>
  </pages>
 </xsl:if>
</xsl:template>

<xsl:template name="numbers">
 <xsl:param name="value"/>
 <xsl:value-of select="translate($value, translate($value, '0123456789', ''), '')"/>
</xsl:template>

<xsl:template match="l:langs">
 <xsl:param name="lang-id"/>
 <xsl:variable name="l" select="key('langs', $lang-id)"/>
 <xsl:if test="$l">
  <xsl:value-of select="$l"/>
 </xsl:if>
 <xsl:if test="not($l)">
  <xsl:value-of select="$lang-id"/>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
